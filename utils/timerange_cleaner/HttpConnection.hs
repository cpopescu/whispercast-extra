module HttpConnection where

import Data.Maybe
import Network.URI
import Network.HTTP

data HttpParams = HttpParams {
  httpParamsPort :: Int,   -- http request are sent to "localhost:port"
  httpParamsAuth :: String -- http authorization header (e.g. "Basic asdfASdfasDFasDFsaD")
} deriving Show

httpPost :: HttpParams -- parameters
         -> String -- url path (e.g. "a/b", then the request is sent to "http://localhost:<httpParamsPort>/a/b")
         -> String -- body
         -> IO (String) -- result body
httpPost params url_path body =
    do resp <- simpleHTTP request
       case resp of
            Left x -> fail $ "simpleHTTP error: " ++ show x
            Right r -> case rspCode r of
                            (2,0,0) -> return $ rspBody r
                            _ -> fail $ "Invalid http code: " ++ show r
    where request = Request {rqURI = uri,
                             rqMethod = POST,
                             rqHeaders = [Header HdrAuthorization (httpParamsAuth params),
                                          Header HdrContentLength (show (length body))],
                             rqBody = body}
          uri = fromJust (parseURI $ "http://localhost:"++(show $ httpParamsPort params)++"/"++url_path)

