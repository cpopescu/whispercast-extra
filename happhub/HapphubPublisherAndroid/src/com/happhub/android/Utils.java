package com.happhub.android;

public class Utils {
  static
  public int compareStrings(final String first, final String second) {
    if ((first == null) ^ (second == null)) {
        return (first == null) ? -1 : 1;
    }
    if ((first == null) && (second == null)) {
        return 0;
    }
    return first.compareTo(second);
  }
  
  static
  public String timestampAsString(long timestamp) {
    timestamp /= 1000; // miliseconds
    
    long ms = timestamp%1000;
    timestamp /= 1000;
    long s = timestamp%60; 
    timestamp /= 60;;
    long m = timestamp;
    
    return m + ":" + ((s < 10) ? "0"+s : s) + ":" + ((ms < 10) ? "00"+ms : ((ms < 100) ? "0"+ms : ms));   
  }
  
}
