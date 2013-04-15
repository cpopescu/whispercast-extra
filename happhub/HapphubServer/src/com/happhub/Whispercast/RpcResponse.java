package com.happhub.Whispercast;

import javax.xml.bind.annotation.XmlRootElement;

@XmlRootElement
public class RpcResponse {
  @XmlRootElement
  static
  public class ResponseHeader {
    public int xid;
    public int msgType;
  };
  public ResponseHeader header;
  
  @XmlRootElement
  static
  public class ResponseBody {
    public int replyStatus;
    
    static
    public class Result {
      public boolean success_;
      public String error_;
    };
    public Result result;
  };
  public ResponseBody rbody;
  
  public RpcResponse() {
  }
};
