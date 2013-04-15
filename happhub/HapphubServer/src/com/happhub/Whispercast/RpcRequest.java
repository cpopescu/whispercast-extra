package com.happhub.Whispercast;

import java.util.List;

import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
class RpcRequest {
  @XmlRootElement
  static
  public class RequestHeader {
    public int xid;
    public int msgType;
  };
  public RequestHeader header;
  
  @XmlRootElement
  static
  public class RequestBody {
    public String service;
    public String method;
    public List<Object> params;
  };
  public RequestBody cbody;
  
  public RpcRequest() {
  }
  public RpcRequest(int xid, String service, String method, List<Object> params) {
    header = new RequestHeader();
    header.msgType = Rpc.MSG_CALL;
    header.xid = xid;
    
    cbody = new RequestBody();
    cbody.service = service;
    cbody.method = method;
    cbody.params = params;
  }
}

