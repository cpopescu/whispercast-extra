module RpcTypes where

import Text.JSON

-- A variant of the regular Text.JSON.valFromObj, useful on optional fields.
optionalValFromObj :: (JSON a) => String -> a -> JSObject JSValue -> Result a
optionalValFromObj name default_value obj = do
  case valFromObj name obj of
    Error _ -> return default_value
    Ok value -> return value

------------------------------------------------------------------------------
-- Base types

data RpcHeader = RpcHeader
    { xid :: Int
    , msgType :: Int
    } deriving (Eq, Show)
data RpcCBody = RpcCBody
    { callService :: String
    , callMethod :: String
    , callParams :: [JSValue] -- list of any values
    } deriving (Eq, Show)
data RpcRBody = RpcRBody
    { replyStatus :: Int
    , replyResult :: JSValue -- any value
    } deriving (Eq, Show)
data RpcQuery = RpcQuery
    { queryHeader :: RpcHeader
    , queryBody :: RpcCBody
    } deriving (Eq, Show)
data RpcReply = RpcReply
    { replyHeader :: RpcHeader
    , replyBody :: RpcRBody
    } deriving (Eq, Show)

mLookup :: String -> [(String, a)] -> Result (a)
mLookup a as = maybe (fail $ "No such element: " ++ a) return (lookup a as)

instance JSON RpcHeader where
    showJSON h = makeObj [ ("xid", showJSON $ xid h)
                         , ("msgType", showJSON $ msgType h)]
    readJSON (JSObject obj) = let jsonObjAssoc = fromJSObject obj
                              in do x <- mLookup "xid" jsonObjAssoc >>= readJSON
                                    t <- mLookup "msgType" jsonObjAssoc >>= readJSON
                                    return RpcHeader { xid = x,
                                                       msgType = t }
    readJSON _ = fail ""

instance JSON RpcCBody where
    showJSON b = makeObj [ ("service", showJSON $ callService b),
                           ("method", showJSON $ callMethod b),
                           ("params", showJSON $ callParams b) ]
    readJSON (JSObject obj) = do service <- valFromObj "service" obj
                                 method <- valFromObj "method" obj
                                 params <- valFromObj "params" obj
                                 return RpcCBody { callService = service,
                                                   callMethod = method,
                                                   callParams = params }
    readJSON _ = fail ""

instance JSON RpcRBody where
    showJSON b = makeObj [ ("replyStatus", showJSON $ replyStatus b),
                           ("result", replyResult b) ]
    readJSON (JSObject obj) = do status <- valFromObj "replyStatus" obj :: Text.JSON.Result (Int)
                                 result <- valFromObj "result" obj :: Text.JSON.Result (JSValue)
                                 return RpcRBody { replyStatus = status,
                                                   replyResult = result}
    readJSON _ = fail ""

instance JSON RpcQuery where
    showJSON q = makeObj [ ("header", showJSON $ queryHeader q),
                           ("cbody", showJSON $ queryBody q) ]
    readJSON (JSObject obj) = do header <- valFromObj "header" obj
                                 body <- valFromObj "cbody" obj
                                 return RpcQuery { queryHeader = header,
                                                   queryBody = body }
    readJSON _ = fail ""

instance JSON RpcReply where
    showJSON r = makeObj [ ("header", showJSON $ replyHeader r),
                           ("rbody", showJSON $ replyBody r) ]
    readJSON (JSObject obj) = do header <- valFromObj "header" obj
                                 body <- valFromObj "rbody" obj
                                 return RpcReply { replyHeader = header,
                                                   replyBody = body }
    readJSON _ = fail ""

rpcTestHeader :: IO ()
rpcTestHeader = do let s = "{\"xid\":1,\"msgType\":1}"
                   case Text.JSON.decode s :: Text.JSON.Result RpcHeader of
                     Error err -> error $ "Failed to decode RpcHeader from: " ++ s ++ ", error: " ++ err
                     Ok header -> print $ "rpcTestHeader: " ++ (show header)
rpcTestCBody :: IO ()
rpcTestCBody = do let s = "{ \"service\" : \"TimeRangeElementService\" , \"method\" : \"GetRange\" , \"params\" : [ 7, \"abc\", false ]}"
                  case Text.JSON.decode s :: Text.JSON.Result RpcCBody of
                    Error err -> error $ "Failed to decode RpcCBody from: " ++ s ++ ", error: " ++ err
                    Ok cbody -> print $ "rpcTestCBody: " ++ (show cbody)
rpcTestRBody :: IO ()
rpcTestRBody = do let s = "{\"replyStatus\":1,\"result\":\"blabla\"}"
                  case Text.JSON.decode s :: Text.JSON.Result RpcRBody of
                    Error err -> error $ "Failed to decode RpcRBody from: " ++ s ++ ", error: " ++ err
                    Ok rbody -> print $ "rpcTestRBody: " ++ (show rbody)
rpcTestQuery :: IO ()
rpcTestQuery = do let s = "{\"header\" : { \"xid\" : 1 , \"msgType\" : 0} , \"cbody\" : { \"service\" : \"TimeRangeElementService\" , \"method\" : \"GetRange\" , \"params\" : [ 7, \"abc\", false ]} }"
                  case Text.JSON.decode s :: Text.JSON.Result RpcQuery of
                    Error err -> error $ "Failed to decode RpcQuery from: " ++ s ++ ", error: " ++ err
                    Ok query -> print $ "rpcTestQuery: " ++ (show query)
rpcTestReply :: IO ()
rpcTestReply = do let s = "{\"header\" : {\"xid\" : 1, \"msgType\" : 1} , \"rbody\" : {\"replyStatus\" : 0 , \"result\" : [52 , \"foo\" , true] }}"
                  case Text.JSON.decode s :: Text.JSON.Result RpcReply of
                    Error err -> error $ "Failed to decode RpcReply from: " ++ s ++ ", error: " ++ err
                    Ok reply -> print $ "rpcTestReply: " ++ (show reply)

-------------------------------------------------------------------------------
-- Factory types

data MediaElementSpecs = MediaElementSpecs
  { elementType :: String
  , elementName :: String
  , elementIsGlobal :: Bool
  , elementDisableRpc :: Bool -- optional
  , elementParams :: String
  } deriving (Eq, Show)
data PolicySpecs = PolicySpecs
  { policyType :: String
  , policyName :: String
  , policyParams :: String
  } deriving (Eq, Show)
data AuthorizerSpecs = AuthorizerSpecs
  { authorizerType :: String
  , authorizerName :: String
  , authorizerParams :: String
  } deriving (Eq, Show)
data ElementConfigurationSpecs = ElementConfigurationSpecs
  { configElements :: [MediaElementSpecs] -- optional
  , configPolicies :: [PolicySpecs] -- optional
  , configAuthorizers :: [AuthorizerSpecs] -- optional
  } deriving (Eq, Show)

instance JSON MediaElementSpecs where
    showJSON b = makeObj [ ("type_", showJSON $ elementType b),
                           ("name_", showJSON $ elementName b),
                           ("is_global_", showJSON $ elementIsGlobal b),
                           ("disable_rpc_", showJSON $ elementDisableRpc b),
                           ("params_", showJSON $ elementParams b) ]
    readJSON (JSObject obj) = do etype <- valFromObj "type_" obj
                                 name <- valFromObj "name_" obj
                                 is_global <- valFromObj "is_global_" obj
                                 disable_rpc <- optionalValFromObj "disable_rpc_" False obj
                                 params <- valFromObj "params_" obj
                                 return MediaElementSpecs { elementType = etype,
                                                            elementName = name,
                                                            elementIsGlobal = is_global,
                                                            elementDisableRpc = disable_rpc,
                                                            elementParams = params }
    readJSON x = fail $ "Expected JSObject, found: " ++ show x

instance JSON PolicySpecs where
    showJSON b = makeObj [ ("type_", showJSON $ policyType b),
                           ("name_", showJSON $ policyName b),
                           ("params_", showJSON $ policyParams b) ]
    readJSON (JSObject obj) = do etype <- valFromObj "type_" obj
                                 name <- valFromObj "name_" obj
                                 params <- valFromObj "params_" obj
                                 return PolicySpecs { policyType = etype,
                                                      policyName = name,
                                                      policyParams = params }
    readJSON x = fail $ "Expected JSObject, found: " ++ show x

instance JSON AuthorizerSpecs where
    showJSON b = makeObj [ ("type_", showJSON $ authorizerType b),
                           ("name_", showJSON $ authorizerName b),
                           ("params_", showJSON $ authorizerParams b) ]
    readJSON (JSObject obj) = do etype <- valFromObj "type_" obj
                                 name <- valFromObj "name_" obj
                                 params <- valFromObj "params_" obj
                                 return AuthorizerSpecs { authorizerType = etype,
                                                          authorizerName = name,
                                                          authorizerParams = params }
    readJSON x = fail $ "Expected JSObject, found: " ++ show x

instance JSON ElementConfigurationSpecs where
    showJSON b = makeObj [ ("elements_", showJSON $ configElements b),
                           ("policies_", showJSON $ configPolicies b),
                           ("authorizers_", showJSON $ configAuthorizers b) ]
    readJSON (JSObject obj) = do elements <- optionalValFromObj "elements_" [] obj
                                 policies <- optionalValFromObj "policies_" [] obj
                                 authorizers <- optionalValFromObj "authorizers_" [] obj
                                 return ElementConfigurationSpecs { configElements = elements,
                                                                    configPolicies = policies,
                                                                    configAuthorizers = authorizers }
    readJSON x = fail $ "Expected JSObject, found: " ++ show x

testMediaElementSpecs :: IO ()
testMediaElementSpecs = do
  let s = "{\"type_\": \"aio_file\", \"name_\": \"aio_events\", \"is_global_\": true , \"disable_rpc_\": false , \"params_\": \"some value\"}"
  case Text.JSON.decode s :: Result MediaElementSpecs of
    Error err -> error $ "Failed to decode MediaElementSpecs from: " ++ s ++ ", error: " ++ err
    Ok x -> print $ "testMediaElementSpecs: " ++ (show x)
testPolicySpecs :: IO ()
testPolicySpecs = do
  let s = "{\"type_\": \"on_command_policy\", \"name_\": \"s1_2_41_switch\", \"params_\": \"some value\"}"
  case Text.JSON.decode s :: Result PolicySpecs of
    Error err -> error $ "Failed to decode PolicySpecs from: " ++ s ++ ", error: " ++ err
    Ok x -> print $ "testPolicySpecs: " ++ (show x)
testAuthorizerSpecs :: IO ()
testAuthorizerSpecs = do
  let s = "{\"type_\": \"simple_authorizer\", \"name_\": \"s1_2_authorizer_preview\", \"params_\": \"{}\"}"
  case Text.JSON.decode s :: Result AuthorizerSpecs of
    Error err -> error $ "Failed to decode testAuthorizerSpecs from: " ++ s ++ ", error: " ++ err
    Ok x -> print $ "testAuthorizerSpecs: " ++ (show x)
testElementConfigurationSpecs :: IO ()
testElementConfigurationSpecs = do
  let s = "{\"elements_\": [{\"type_\": \"aio_file\", \"name_\": \"aio_events\", \"is_global_\": true , \"disable_rpc_\": false , \"params_\": \"some value\"}, " ++
                           "{\"type_\": \"aio_file\", \"name_\": \"aio_events\", \"is_global_\": true , \"disable_rpc_\": false , \"params_\": \"some value\"}, " ++
                           "{\"type_\": \"aio_file\", \"name_\": \"aio_events\", \"is_global_\": true , \"disable_rpc_\": false , \"params_\": \"some value\"}], " ++
           "\"policies_\": [{\"type_\": \"on_command_policy\", \"name_\": \"s1_2_41_switch\", \"params_\": \"some value\"}, " ++
                           "{\"type_\": \"on_command_policy\", \"name_\": \"s1_2_41_switch\", \"params_\": \"some value\"}], " ++
           "\"authorizers_\": [{\"type_\": \"simple_authorizer\", \"name_\": \"s1_2_authorizer_preview\", \"params_\": \"{}\"}]}"
  case Text.JSON.decode s :: Result ElementConfigurationSpecs of
    Error err -> error $ "Failed to decode testElementConfigurationSpecs from: " ++ s ++ ", error: " ++ err
    Ok x -> print $ "testElementConfigurationSpecs: " ++ (show x)

