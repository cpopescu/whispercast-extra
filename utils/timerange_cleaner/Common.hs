module Common where 

import Monad (liftM)
import Control.Monad (filterM)
import Data.Char (isSpace)
import Text.Printf(printf)
import System.FilePath.Posix (joinPath, takeDirectory)
import System.Directory
import System.Posix.Files(rename, getFileStatus, fileSize)

-- conditional/ternary operator
cond :: Bool -> a -> a -> a
cond True x y = x
cond False x y = y

condM :: (Monad m) => Bool -> m a -> m a -> m a
condM True x y = x
condM False x y = y

-- The 'concatMapM' function generalizes 'concatMap' to arbitrary monads.
concatMapM :: (Monad m) => (a -> m [b]) -> [a] -> m [b]
concatMapM f xs = liftM concat (mapM f xs)

-- Join paths. e.g. "a" "b" => "a/b"
joinPaths :: String -> String -> String
joinPaths a b = joinPath [a,b]

-- trim whitespaces
trim :: String -> String
trim = f . f
   where f = reverse . dropWhile isSpace

-- tries to rename the given file.
-- If unsuccessful, then it does: copy + delete
forceRenameFile :: FilePath -> FilePath -> IO()
forceRenameFile a b = do
  catch (renameFile a b)
        (\e -> do copyFile a b
                  removeFile a)

-- move file
-- e.g. moveFile "/tmp/x.flv" "/var/y.flv"
moveFile :: FilePath -> FilePath -> IO()
moveFile a b = do
  putStrLn $ "Moving: " ++ a 
  putStrLn $ "    To: " ++ b
  createDirectoryIfMissing True (takeDirectory b)
  forceRenameFile a b

deleteFile :: FilePath -> IO()
deleteFile f = do
  putStrLn $ "Deleting: " ++ f
  removeFile f

-- merge by moving the content of directory a, into directory b
-- e.g.: mergeMoveDirectories "/tmp/a" "/home/b" -- will move: "/tmp/a" over "/home/b". And "/tmp/a" dissapears.
mergeMoveDirectories :: FilePath -> FilePath -> IO ()
mergeMoveDirectories a b = do
  a_is_file <- doesFileExist a
  b_exists <- doesDirectoryExist b
  if a_is_file || (not b_exists)
     then do rename a b -- works with both: file & directory
     else do items <- getDirectoryContents a >>= filterM (\f -> return $ f /= "." && f /= "..")
             mapM_ (\f -> mergeMoveDirectories (joinPaths a f) (joinPaths b f)) items

-- merge by copying the content of directory a, into directory b
-- e.g.: mergeCopyDirectories "/tmp/a" "/home/b" -- will copy: "/tmp/a" over "/home/b". And "/tmp/a" remains intact.
mergeCopyDirectories :: FilePath -> FilePath -> IO()
mergeCopyDirectories a b = do
  a_is_file <- doesFileExist a
  if a_is_file
     then do putStrLn $ "Copy: " ++ a
             putStrLn $ "  To: " ++ b
             copyFile a b
     else do createDirectoryIfMissing False b
             items <- getDirectoryContents a >>= filterM (\f -> return $ f /= "." && f /= "..")
             mapM_ (\f -> mergeCopyDirectories (joinPaths a f) (joinPaths b f)) items


-- returns file size in bytes
getFileSize :: FilePath -> IO(Integer)
getFileSize f = do getFileStatus f >>= return . fromIntegral . fileSize

-- Returns a human readable byte size.
-- e.g. 1024*1024*3.14 => "3.14 MB"
formatBytes :: Integer -> String
formatBytes bytes
    | bytes >= kGB = printf "%.2f GB" $ fbytes / fromIntegral kGB
    | bytes >= kMB = printf "%.2f MB" $ fbytes / fromIntegral kMB
    | bytes >= kKB = printf "%.2f KB" $ fbytes / fromIntegral kKB
    | otherwise = show bytes ++ " bytes"
  where
    kKB = 1024
    kMB = 1024*1024
    kGB = 1024*1024*1024
    fbytes = fromIntegral bytes :: Float

