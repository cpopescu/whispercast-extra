module Flags where

-- Command line arguments

import System
import System.Exit
import System.IO
import System.IO.Error
import System.FilePath.Posix
import System.Console.GetOpt
import Directory (doesFileExist, doesDirectoryExist)
import Monad (unless, when)
import Data.List.Split (splitOn)
import Text.Regex (mkRegex, splitRegex, subRegex)
import Data.List (lines, isPrefixOf, intercalate)

import Common

data Flags = Flags {
  flagWhispercastPort :: Int,
  flagWhispercastPass :: String,
  flagMediaDir :: String,
  flagBackupDir :: String,
  flagRanges :: [String],
  flagDaysOld :: Int,
  flagSimulate :: Bool
} deriving (Show)

defaultFlags = Flags {
  flagWhispercastPort = 8180,
  flagWhispercastPass = "",
  flagMediaDir = "",
  flagBackupDir = "",
  flagRanges = [],
  flagDaysOld = 365,
  flagSimulate = False
}

-- NOTE: "IO Flags" actions are not required, but it's simpler on some parameter
-- cases (e.g. -v, -h) to take a quick action (e.g. print version, print help)
-- then immediately exit.
-- Every option is actually a function, which takes an Flags struct, sets
-- the corresponding option and returns a new Flags. These functions can be
-- chain called (through >>=) to gather all options in 1 single final Flags.
options :: [OptDescr (Flags -> IO Flags)]
options = [
    Option ['v'] ["version"]    (NoArg showVersion)  "show version number",
    Option ['h'] ["help"]       (NoArg showHelp)     "show help",
    Option ['p'] ["wport"]      (ReqArg (\ a o -> return o {flagWhispercastPort = read a}) "Port") "Whispercast port. The host is: localhost.",
    Option ['a'] ["wpass"]      (ReqArg (\ a o -> return o {flagWhispercastPass = a}) "WhispercastAdminPass") "Whispercast authentication, admin password.",
    Option ['d'] ["media_dir"]  (ReqArg (\ a o -> return o {flagMediaDir = a}) "MediaDir") "Directory containing ranger dirs (e.g.: '.../0/private/live/')",
    Option ['b'] ["backup_dir"] (ReqArg (\ a o -> return o {flagBackupDir = a}) "BackupDir") "Instead of deleting files, move them here. If not specified then media files are simply deleted.",
    Option [   ] ["ranges"]     (ReqArg (\ a o -> return o {flagRanges = splitRanges a}) "TimeRanges") "The list of timerange elements to clean. If not specified then all ranges on local whispercast are cleaned.",
    Option [   ] ["days_old"]   (ReqArg (\ a o -> return o {flagDaysOld = read a}) "DaysOld") "Clean only files older than this many days.",
    Option ['s'] ["simulate"]   (NoArg  (\   o -> return o {flagSimulate = True})) "Don't move/delete/modify any files. Just simulate."
  ]

-- Splits the list of timeranges.
-- e.g. "s3_2_81_ranger , s1_7_14_ranger" -> ["s3_2_81_ranger", "s1_7_14_ranger"]
splitRanges :: String -> [String]
splitRanges = map trim . splitRegex (mkRegex " *, *")

-- Action in case of -v option
showVersion :: Flags -> IO Flags
showVersion _ = do
  putStrLn "Version: 1.0"
  exitWith ExitSuccess

-- Action in case of -h option
showHelp :: Flags -> IO Flags
showHelp _ = do
  putStrLn $ usageInfo "Usage: main [OPTION...]" options
  exitWith ExitSuccess

-- simply use getOpt on the program arguments
parseFlags :: [String] -- program arguments, usually getArgs
             -> IO [Flags -> IO Flags] -- list of option setters. Each element sets an option;
parseFlags args = case getOpt RequireOrder options args of
                   ([],   [], [])   -> do { putStrLn $ usageInfo "Usage: main [OPTION...]" options; exitWith ExitSuccess; }
                   (opts, [], [])   -> return opts
                   (_,   lst, [])   -> fail $ "Unrecognized options: " ++ unwords lst
                   (_,    _,  errs) -> fail $ "getOpt failed: " ++ concat errs

-- chain call all option functions, gathering so all options in a single Flags
processFlags :: [Flags -> IO Flags] -> IO Flags
processFlags flags = foldl (>>=) (return defaultFlags) flags

-- check if the given directory exists. Throws an IOError if it does not.
-- e.g. checkDirectoryExists "MediaDirectory" "/tmp/foo"
--      => Exception: MediaDirectory: /tmp/foo does not exist
checkDirectoryExists :: String -> FilePath -> IO()
checkDirectoryExists description path = do
  is_dir <- doesDirectoryExist path
  unless is_dir $ do
    ioError $ mkIOError doesNotExistErrorType description Nothing (Just path)

-- validate flags
checkFlags :: Flags -> IO()
checkFlags flags = do
  checkDirectoryExists "MediaDir" (flagMediaDir flags :: FilePath)
  --checkDirectoryExists "BackupDir" (flagBackupDir flags :: FilePath)

-- remove comment (everything following '#')
-- e.g. "abc#def" => "abc"
cutComment :: String -> String
cutComment s = subRegex (mkRegex "#.*") s ""

-- reads the flag file, returns the list of flags
-- read the whole file -> split lines -> cut comments -> trim whitespaces -> filter out empty lines
readFlagsFile :: String -> IO [String]
readFlagsFile file = (readFile file) >>= (return . filter (/= "") . map (trim . cutComment) . lines)

-- if file does exist => returns the list of lines in the file
-- else => return [file]
tryReadFlagsFile :: String -> IO [String]
tryReadFlagsFile file = do file_exists <- doesFileExist file
                           if file_exists then readFlagsFile file
                           else return [file]

-- replace arguments that identify files with file lines
getArguments :: IO [String]
getArguments = getArgs >>= concatMapM tryReadFlagsFile

-- parse program arguments into a Flags object, validate & return the flags
getFlags :: IO (Flags)
getFlags = do args <- getArguments
              putStrLn $ "args: " ++ show args
              flags <- parseFlags args >>= processFlags
              putStrLn $ show flags
              putStrLn ""
              checkFlags flags
              return flags


writeFlagsToFile :: Flags -> FilePath -> IO()
writeFlagsToFile flags filename = withFile filename WriteMode
    (\h -> do hPutStrLn h ("--wport=" ++ show (flagWhispercastPort flags))
              hPutStrLn h ("--wpass=" ++ flagWhispercastPass flags)
              hPutStrLn h ("--media_dir=" ++ flagMediaDir flags)
              hPutStrLn h ("--backup_dir=" ++ flagBackupDir flags)
              hPutStrLn h ("--days_old=" ++ show (flagDaysOld flags))
              hPutStrLn h ("--ranges=" ++ intercalate ", " (flagRanges flags))
              when (flagSimulate flags) $ hPutStrLn h ("--s")
              hPutStrLn h "")

