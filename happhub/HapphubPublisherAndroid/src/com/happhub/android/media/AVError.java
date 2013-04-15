package com.happhub.android.media;

public class AVError {
  static final
  public int EXIT = Code('E','X','I','T');
  static final
  public int EOF = Code('E','O','F',' ');
  
  static final
  public int UNKNOWN = Code('U','N','K','N');
  
  static final
  public int INVALID_DATA = Code('I','N','D','A');
  static final
  public int BUFFER_TOO_SMALL = Code('B','U','F','S');
  
  static final
  public int EPERM            = -1;      /* Operation not permitted */
  static final
  public int ENOENT           = -2;      /* No such file or directory */
  static final
  public int ESRCH            = -3;      /* No such process */
  static final
  public int EINTR            = -4;      /* Interrupted system call */
  static final
  public int EIO              = -5;      /* I/O error */
  static final
  public int ENXIO            = -6;      /* No such device or address */
  static final
  public int E2BIG            = -7;      /* Argument list too long */
  static final
  public int ENOEXEC          = -8;      /* Exec format error */
  static final
  public int EBADF            = -9;      /* Bad file number */
  static final
  public int ECHILD          = -10;      /* No child processes */
  static final
  public int EAGAIN          = -11;      /* Try again */
  static final
  public int ENOMEM          = -12;      /* Out of memory */
  static final
  public int EACCES          = -13;      /* Permission denied */
  static final
  public int EFAULT          = -14;      /* Bad address */
  static final
  public int ENOTBLK         = -15;      /* Block device required */
  static final
  public int EBUSY           = -16;      /* Device or resource busy */
  static final
  public int EEXIST          = -17;      /* File exists */
  static final
  public int EXDEV           = -18;      /* Cross-device link */
  static final
  public int ENODEV          = -19;      /* No such device */
  static final
  public int ENOTDIR         = -20;      /* Not a directory */
  static final
  public int EISDIR          = -21;      /* Is a directory */
  static final
  public int EINVAL          = -22;      /* Invalid argument */
  static final
  public int ENFILE          = -23;      /* File table overflow */
  static final
  public int EMFILE          = -24;      /* Too many open files */
  static final
  public int ENOTTY          = -25;      /* Not a typewriter */
  static final
  public int ETXTBSY         = -26;      /* Text file busy */
  static final
  public int EFBIG           = -27;      /* File too large */
  static final
  public int ENOSPC          = -28;      /* No space left on device */
  static final
  public int ESPIPE          = -29;      /* Illegal seek */
  static final
  public int EROFS           = -30;      /* Read-only file system */
  static final
  public int EMLINK          = -31;      /* Too many links */
  static final
  public int EPIPE           = -32;      /* Broken pipe */
  static final
  public int EDOM            = -33;      /* Math argument out of domain of func */
  static final
  public int ERANGE          = -34;      /* Math result not representable */
  
  static
  public int Code(byte a, byte b, byte c, byte d) {
	  return ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24));
  }
  static
  public int Code(char a, char b, char c, char d) {
	  return ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24));
  }
}
