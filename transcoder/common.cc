#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h> // for gethostbyname

#include "common.h"
#include "common/base/strutil.h"
#include "common/io/ioutil.h"


namespace manager {

int64 ReadIdFromFilename(const std::string & filename) {
  if(filename.size() == 0) {
    return -1;
  }
  int start_pos = filename.rfind('/');
  start_pos = (start_pos == std::string::npos) ? 0 : start_pos + 1;
  int end_pos = filename.find('.', start_pos);
  end_pos = (end_pos == std::string::npos) ? filename.size() : end_pos;
  if(end_pos <= start_pos) {
    // matches: "/.avi"
    return -1;
  }
  int len = end_pos - start_pos;

  const std::string strid = filename.substr(start_pos, len);
  int64 id = ::atoll(strid.c_str());
  if(id == 0 && strid.c_str()[0] != '0') {
    return -1;
  }
  return id;
}

std::string RelativePath(const std::string & big_path,
                         const std::string & sub_path) {
  if ( !strutil::StrStartsWith(big_path, sub_path) ) {
    return big_path;
  }
  if ( sub_path.empty() ) {
    return big_path;
  }
  CHECK_GE(sub_path.size(), 1);
  int sub_path_size = (*sub_path.rbegin()) == '/' ?
                        sub_path.size() - 1 :
                        sub_path.size();
  const char * cstr_relative_path = big_path.c_str() + sub_path_size;
  if ( *cstr_relative_path != '/' ) {
    // big_path = "/a/bc"
    // sub_path = "/a/b"
    return big_path;
  }
  cstr_relative_path++;
  return std::string(cstr_relative_path);
}

bool TryRename(const std::string& src, std::string& dst,
               uint32 max_retry) {
  if ( !io::IsDir(strutil::Dirname(dst)) ) {
    // early fail, no need for tries
    LOG_ERROR << "Cannot rename src: [" << src << "] to dst: [" << dst << "]"
                 ", destination dir does not exist";
    return false;
  }

  const std::string dst_without_extension = strutil::CutExtension(dst);
  const std::string dst_extension = strutil::Extension(dst);
  const bool has_extension = (dst_without_extension != dst);

  std::string dst_try = dst;

  unsigned i = 1;
  while ( i < max_retry ) {
    if ( io::Rename(src, dst_try, false) ) {
      break;
    }
    dst_try = (has_extension ?
               strutil::StringPrintf("%s(%d).%s", dst_without_extension.c_str(),
                                     i, dst_extension.c_str()) :
               strutil::StringPrintf("%s(%d)", dst_without_extension.c_str(), i));
    i++;
  }
  if ( i == max_retry ) {
    return false;
  }
  dst = dst_try;
  return true;
}

std::string MakeScpPath(const std::string& username,
                        const net::IpAddress& ip,
                        const std::string& path) {
  return username + "@" + ip.ToString() + ":" + path;
}

bool IsScpPath(const std::string& path) {
  return path.find('@') != string::npos;
}

int32 ExternalIpAddress() {
  // TODO(cosmin): Code is good, but it's not working
  char hostname[128] = {0};
  if ( 0 != ::gethostname(hostname, sizeof(hostname)) ) {
    LOG_ERROR << "::gethostname failed: " << GetLastSystemErrorDescription();
    return -1;
  }
  LOG_WARNING << "ExternalIpAddress hostname: [" << hostname << "]";
  const struct hostent * he = ::gethostbyname(hostname);
  if ( he == NULL ) {
    LOG_ERROR << "::gethostbyname failed: " << GetLastSystemErrorDescription();
    return -1;
  }
  const char * haddr = he->h_addr_list[0];
  for ( int i = 0; he->h_addr_list[i] != NULL; i++ ) {
    LOG_WARNING << "ExternalIpAddress haddr[" << i << "]: "
                << (int)(he->h_addr_list[i][0]) << "."
                << (int)(he->h_addr_list[i][1]) << "."
                << (int)(he->h_addr_list[i][2]) << "."
                << (int)(he->h_addr_list[i][3]);
  }
  int32 ip = (static_cast<unsigned int>(haddr[0]) << 24 |
              static_cast<unsigned int>(haddr[1]) << 16 |
              static_cast<unsigned int>(haddr[2]) <<  8 |
              static_cast<unsigned int>(haddr[3]) <<  0);
  LOG_WARNING << "ExternalIpAddress found: " << net::IpAddress(ip);
  return ip;
}
};
