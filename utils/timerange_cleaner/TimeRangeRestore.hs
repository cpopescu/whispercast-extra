
import System
import System.FilePath.Posix (joinPath)
import Monad(when)
import Directory (doesFileExist)

import Flags
import Common


main :: IO()
main = do args <- getArgs
          when ((length args) /= 1) $ do
            prog_name <- getProgName
            putStrLn $ "Usage: " ++ prog_name ++ " <backup dir>"
            putStrLn $ " <backup_dir>: a backup created by TimeRangeCleaner, e.g.: \".../timerange_cleaner_backup/20120321-182454\""
            exitWith ExitSuccess
          let backup_dir = args !! 0
          putStrLn $ "Restoring backup: " ++ show backup_dir
          putStrLn ""

          flags <- readFlagsFile (joinPaths backup_dir "flags.txt") >>= parseFlags >>= processFlags
          putStrLn $ "Backup flags: " ++ show flags
          putStrLn ""
          checkFlags flags

          -- put back deleted files
          let media_dir = flagMediaDir flags
          mergeCopyDirectories (joinPaths backup_dir "deleted") media_dir

          -- delete created files
          --let created_file = joinPaths backup_dir "created.txt"
          --created_file_exists <- doesFileExist created_file
          --when created_file_exists $ do
          --  readFile created_file >>= (return . lines) >>= (mapM_ $ deleteFile . joinPaths media_dir)

          print "Success"

