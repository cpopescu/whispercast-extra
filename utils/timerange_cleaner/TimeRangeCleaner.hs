
-- dependency packages: json, http, filepath, split

import System
import System.FilePath.Posix (joinPath)
import System.Directory (doesDirectoryExist, createDirectory, getDirectoryContents, doesFileExist)
import Monad (unless)
import Control.Monad (filterM)
import Text.Regex
import Data.Maybe
import Data.Foldable(foldlM)
import List (sort)
import qualified Codec.Binary.Base64.String as Base64

import System.Locale (defaultTimeLocale)
import Data.Time (getCurrentTime)
import Data.Time.Clock.POSIX (utcTimeToPOSIXSeconds, getPOSIXTime)
import Data.Time.Clock (UTCTime)
import Data.Time.Format (parseTime, formatTime)

import Common
import Flags
import HttpConnection
import RpcTypes
import RpcQueries

-- Compose source element name, from timerange element name.
-- e.g. "s1_2_42_ranger" => "s1_source_42"
sourceNameFromTimerange :: String -> String
sourceNameFromTimerange timerange = makeSourcename $ splitRegex (mkRegex "_") timerange
  where
    makeSourcename :: [String] -> String
    makeSourcename [s,n1,n2,"ranger"] = s++"_source_"++n2
    makeSourcename lst = error $ "Invalid timerange element name: " ++ show lst

-- Filter function for valid media file names.
-- e.g. "media_20111207-153241-695_20111207-153456-760.flv" => True
--      "blablabla" => False
isValidMediaFile :: String -> Bool
isValidMediaFile file = isCompleteMatch $ matchRegexAll (mkRegex "media_[0-9]{8}-[0-9]{6}-[0-9]{3}_[0-9]{8}-[0-9]{6}-[0-9]{3}\\.flv") file
  where isCompleteMatch :: (Maybe (String, String, String, [String])) -> Bool
        isCompleteMatch (Just ("",file,"",[])) = True
        isCompleteMatch _ = False

-- Parse a media filename into (start timestamp, end timestamp). Each timestamp is seconds from Epoch.
-- e.g. "media_20111207-153241-695_20111207-153456-760.flv" => (1323271961, 1323272096)
parseMediaFile :: String -> (Integer, Integer)
parseMediaFile file = makeTimestamps $ splitRegex (mkRegex "_") file
  where
    makeTimestamps :: [String] -> (Integer,Integer)
    makeTimestamps [_, start, end] = (parseTS start, parseTS end)
    makeTimestamps lst = error $ "Illegal filename: " ++ (show lst)
    parseTS :: String -> Integer
    -- parseTime cannot deal with milliseconds, the purpose of 'take 15' is to substring cut out the milliseconds
    parseTS s = round $ utcTimeToPOSIXSeconds $ fromJust (parseTime defaultTimeLocale "%Y%m%d-%H%M%S" (take 15 s) :: Maybe UTCTime)

-- intersect ranges, returns the overlapping elements
intersectRanges :: [(String, (Integer, Integer))] -- range 1
                -> [(String, (Integer, Integer))] -- range 2
                -> ([(String, (Integer, Integer))], -- elements of range2 which overlap with sth in range1
                    [(String, (Integer, Integer))]) -- elements of range2 which don't overlap with anything in range1
intersectRanges range1 range2 = intersectRangesRec range1 range2 ([], [])
  where
    intersectRangesRec  :: [(String, (Integer, Integer))] -- range 1
                        -> [(String, (Integer, Integer))] -- range 2
                        -> ([(String, (Integer, Integer))], [(String, (Integer, Integer))]) -- accumulator
                        -> ([(String, (Integer, Integer))], [(String, (Integer, Integer))]) -- result
    intersectRangesRec _ [] acc = acc
    intersectRangesRec range1 (e:es) (acc_overlap, acc_independent)
      | isOverlapping range1 e = intersectRangesRec range1 es (e:acc_overlap, acc_independent)
      | otherwise = intersectRangesRec range1 es (acc_overlap, e:acc_independent)
    isOverlapping :: [(String, (Integer, Integer))] -> (String, (Integer, Integer)) -> Bool
    isOverlapping [] _ = False
    isOverlapping ((_, (xstart, xend)):xs) e@(_, (estart, eend))
      | (xstart <= estart && estart < xend) || (xstart < eend && eend <= xend) = True
      | otherwise = isOverlapping xs e

-- delete/move file
cleanFile :: String -- backup directory (if not "" then move files here instead of deleting them)
          -> String -- media directory (contains filename)
          -> String -- filename (relative to media directory)
          -> IO()
cleanFile backup_dir media_dir f
  | backup_dir == "" = deleteFile (joinPaths media_dir f)
  | otherwise = moveFile (joinPaths media_dir f) (joinPath [backup_dir, "deleted", f])

-- cleans a timerange element
cleanRange :: HttpParams -- HTTP authentication + other params
           -> String -- media dir
           -> String -- backup dir
           -> Int -- days old, clear files older than these many days
           -> Bool   -- simulate (== don't delete/modify any files, just show the findings)
           -> String -- TimeRanger element name (e.g.: "s3_2_87_ranger")
           -> IO (Integer) -- returns the number of BYTES cleared or which would be cleared (in case of simulate == true)
cleanRange hp media_dir backup_dir days_old simulate timerange = do
  putStrLn ""
  putStrLn $ "Cleaning " ++ show timerange

  -- list range names for the given element
  range_names <- rpcGetAllRangeNames hp timerange
  putStrLn $ "range_names: " ++ show range_names

  -- obtain the list of range intervals: [(start,end)]
  range_intervals <- mapM (rpcGetRange hp timerange) range_names
  -- print the obtained list of ranges
  mapM_ (\(name,(start,end)) -> putStrLn $ "Range " ++ name ++ " -> " ++ show start ++ " - " ++ show end)
        (zip range_names range_intervals)

  -- list media files for the given element
  let mdir = joinPath [media_dir, sourceNameFromTimerange timerange, "media"]
  -- getDirectoryContents: returns all directory contents: including files, dirs, '.', '..'. Relative to mdir.
  -- doesFileExist: filter dirs, keep only files
  -- isValidMediaFile: filter invalid file names (valid format e.g. "media_20111207-153241-695_20111207-153456-760.flv")
  -- sort: lexicographical order
  all_files <- getDirectoryContents mdir >>= filterM (doesFileExist . (joinPaths mdir)) >>= filterM (return . isValidMediaFile) >>= (return . sort)
  -- parse filenames into [(start ts, end ts)]
  let all_file_intervals = map parseMediaFile all_files
  
  -- find which files are old enough to be cleaned, filter out new files
  now_ts <- getPOSIXTime >>= (return . floor) -- seconds from EPOCH
  let old_ts = now_ts - (toInteger days_old) * 3600 * 24 -- files before this ts are safe to clean, newer files must not be modified!
  let (files, file_intervals) = unzip $ filter (\(file,(start_ts, end_ts)) -> end_ts < old_ts)
                                               (zip all_files all_file_intervals)
   
  -- match ranges to files, find used/unused files
  let (used_files, unused_files) = intersectRanges (zip range_names range_intervals) (zip files file_intervals)

  putStrLn $ "Files in [" ++ mdir ++ "]: #" ++ show (length files) ++ " old files" ++
             " (" ++ show ((length all_files) - (length files)) ++ " files younger than " ++ show days_old ++ " days are hidden)"
  mapM_ (\ f@(file, (start, end)) ->
              putStrLn $ " - " ++ file ++ " => " ++ show start ++ " : " ++ show end ++
                         " # " ++ (cond (elem (file,(start,end)) used_files) "USED" "free"))
        (zip files file_intervals)

  -- compute cleanup files size
  bytes <- mapM (getFileSize . (joinPaths mdir)) (fst (unzip unused_files)) >>= (return . (foldl (+) 0))
  putStrLn $ "Cleanup size: " ++ formatBytes bytes

  if simulate then
    putStrLn "Stopping Simulation"
    -- don't use exitWith(), because clean() may be called multiple times with various timeranges
  else
    -- clean files
    mapM_ (cleanFile backup_dir media_dir)
          [joinPath [sourceNameFromTimerange timerange, "media", f] | f <- (fst $ unzip unused_files)]  -- filenames relative to flagMediaDir

  return bytes

main :: IO ()
main = do internalTests
          flags <- getFlags -- program arguments in a nice structure
          now <- getCurrentTime
          
          -- backup to: <flagBackupDir>/<date>/...
          let backup_dir = cond (flagBackupDir flags == "")
                                "" $ joinPaths (flagBackupDir flags) (formatTime defaultTimeLocale "%Y%m%d-%H%M%S" now)
          putStrLn $ "Backup dir: [" ++ backup_dir ++ "]"
          unless (backup_dir == "") $ do
            createDirectory backup_dir -- may throw "already exists" exception, which is excelent because cumulating backups creates a mess
            -- save flags
            writeFlagsToFile flags $ joinPaths backup_dir "flags.txt"

          let hp = HttpParams { httpParamsPort = flagWhispercastPort flags,
                                httpParamsAuth = "Basic " ++ (Base64.encode ("admin:" ++ (flagWhispercastPass flags))) }

          -- if no ranges specified, then obtain all ranges on local whispercast
          ranges <- condM (flagRanges flags == []) (getAllRangeElements hp) (return $ flagRanges flags)

          -- clean each and every range. Returns the vector of cleaned files size.
          cleanup_sizes <-
            mapM (cleanRange hp
                             (flagMediaDir flags)
                             backup_dir
                             (flagDaysOld flags)
                             (flagSimulate flags)) ranges

          let bytes = foldl (+) 0 cleanup_sizes
          putStrLn ""
          putStrLn $ "Total Cleanup: " ++ formatBytes bytes
          print "Success"

internalTests :: IO()
internalTests = do
  putStrLn "### Running tests..."
  rpcTestHeader
  rpcTestCBody
  rpcTestRBody
  rpcTestQuery
  rpcTestReply
  testMediaElementSpecs
  testPolicySpecs 
  testAuthorizerSpecs
  testElementConfigurationSpecs
  putStrLn "#### Tests Done."
  putStrLn ""

