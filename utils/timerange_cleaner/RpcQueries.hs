module RpcQueries where

import Text.JSON
import RpcTypes
import HttpConnection
import Text.Regex (mkRegex, splitRegex)

rpcQuery :: HttpParams -- Http parameters
         -> String -- URL
         -> String -- Service
         -> String -- Method
         -> [JSValue] -- Parameters
         -> IO(JSValue) -- Result
rpcQuery hp url_path service method params =
  do let q = RpcQuery { queryHeader = RpcHeader { xid = 1, msgType = 0},
                        queryBody = RpcCBody { callService = service,
                                               callMethod = method,
                                               callParams = params } }
     let json = Text.JSON.encode q :: String
     response <- catch (httpPost hp url_path json)
                       (\ e -> fail $ "rpcQuery Failed: " ++ show e ++ ", hp: " ++ show hp ++ ", url_path: " ++ url_path ++
                                      ", service: " ++ service ++ ", method: " ++ method ++ ", params: " ++ show params)
     case Text.JSON.decode response :: Text.JSON.Result (RpcReply) of
       Error err -> fail $ "Failed to decode RpcReply from: " ++ response
       Ok r -> return $ replyResult $ replyBody r

rpcGetAllRangeNames :: HttpParams -- http parameters
                    -> String -- element name (e.g.: "s3_2_87_ranger")
                    -> IO ([String]) -- returns ranges names (e.g.: ["52", "43", "2342"])
rpcGetAllRangeNames hp timerange =
  do let url_path = "rpc-config/extra_library/"++timerange
     json_result <- rpcQuery hp url_path "TimeRangeElementService" "GetAllRangeNames" []
     case (readJSON json_result :: Result [String]) of
       Error err -> fail $ "expected [String], found: " ++ show json_result
       Ok result -> return result

rpcGetRange :: HttpParams -- http parameters
            -> String -- element name (e.g.: "s3_2_87_ranger")
            -> String -- range name (e.g.: "52")
            -> IO ((Integer, Integer)) -- returns range interval (e.g. (1323269130,1323269740))
rpcGetRange hp element range = 
  do let url_path = "rpc-config/extra_library/"++element
     json_result <- rpcQuery hp url_path "TimeRangeElementService" "GetRange" [showJSON range]
     case (readJSON json_result :: Result String) of
       Error err -> fail $ "expected String result, found: " ++ show json_result
       Ok result -> return $ parseInterval result
  where
    parseInterval :: String -> (Integer, Integer)
    parseInterval str = makeTuple $ splitRegex (mkRegex "/") str
    makeTuple :: [String] -> (Integer, Integer)
    makeTuple [a,b] = (read a, read b)
    makeTuple lst = error $ "invalid list: " ++ show lst

getAllRangeElements :: HttpParams -- http parameters
                    -> IO([String]) -- returns all TimeRangeElement elements on local whispercast (e.g. ["s1_2_42_ranger", "s1_7_13_ranger"])
getAllRangeElements hp =
  do let url_path = "rpc-config"
     json_result <- rpcQuery hp url_path "MediaElementService" "GetAllElementConfig" []
     case (readJSON json_result :: Result ElementConfigurationSpecs) of
       Error err -> fail $ "expected JSValue, found: " ++ show err
       Ok specs -> do let eee = map elementName $ filter (\e -> elementType e == "time_range") (configElements specs)
                      putStrLn $ "getAllRangeElements => " ++ show eee
                      return eee

