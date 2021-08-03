/**************************************************************************
 *
 * Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
 * All rights reserved.
 *
 * Distributed under the terms of the GNU General Public License v2.
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 *
 **************************************************************************/
#ifndef NETWORK_DYN_DDNS_H_
#define NETWORK_DYN_DDNS_H_

#include <string>
#include <base/base64.h>
#include <base/logging.h>
#include <network/sock/inet_address.h>
#include <network/sock/connector.h>
#include <network/sock/callback.h>
#include <network/event/event_loop.h>

namespace MSF {

static const char *kDefaultDynDDNSCheckIP =
    "checkip.dyndns.com"; /* default check ip address */
static const std::string kDefaultDynDDNSUpdate =
    "members.dyndns.org"; /* default update address */

#define DDNSOK 0          /* Process return OK */
#define DDNSERROR (-1)    /* Process return ERROR */
#define DDNSPAMERROR (-2) /* Parameter error */
#define DDNSNOMEM (-3)    /* No enough memory for allocate */

enum DDNSError {
  kDDNSuccess = 0,
  kDDNSError = -1,
  kDDNSParamError = -2,
  kDDNSNoMemory = 3
};

enum DDNState {
  kDDNStateGetIPRequest = 0,
  kDDNStateGetIPResponse = 1,
};

class DDNS {
 public:
  DDNS(EventLoop *loop, const InetAddress &addr)
      : loop_(loop), server_addr_(addr) {}
  virtual ~DDNS() = default;

  uint32_t ip_check_peroid() const noexcept { return ip_check_peroid_; }
  uint32_t nop_retry_times() const noexcept { return nop_retry_times_; }
  const std::string &user_agent() const noexcept { return user_agent_; }

  virtual bool IPCheck();
  virtual bool GetIPAddress();
  virtual bool Start();

 protected:
  uint32_t ip_check_peroid_ = 60;      /* IP check period */
  uint32_t nop_retry_times_ = 3;       /* HeartBeat retry times */
  std::string user_agent_ = "luotang"; /* User-Agent specified */

  std::string company_ = "luotang.me"; /* Company */
  std::string device_ = "dev";         /* Device */
  std::string version = "v1.0";        /* User contact info */

  EventLoop *loop_;
  InetAddress server_addr_;
  ConnectorPtr ctor_;
  std::string user_name_;
  std::string pass_word_;
  std::string domain_;
  int system_;
  int wildcard_;
  int offline_;

  DDNState state_ = kDDNStateGetIPRequest;

  InetAddress local_older_addr_;
  InetAddress local_fresh_addr_;
  InetAddress outbound_older_addr_;
  InetAddress outbound_fresh_addr_;
};

/* system parameters  */
#define DYNDNS 0
#define STATDNS 1
#define CUSTOM 2

struct DDNSInfo {
  std::string hostname_; /* 0,1,2,3 */
  std::string myip_;     /* 0,1 and 3's ip1 */
  std::string wildcard_; /* 0,1 */
  std::string mx_;       /* 0,1 */
  std::string backmx_;   /* 0,1 */
  std::string offline_;  /* 0,1 */
};

/* Upeate Syntax Errors */
#define BADSYS 1 /* The system parameter given was not valid */
#define BADAGENT                                               \
  2 /* The useragent your client send has been blocked for not \
       */
/* following these specifications, or no user agent was specified. */
/* Account-Related Errors */
#define BADAUTH 3 /* The username or password specified are incorrect */
#define NOTDONATOR \
  4 /* The offline setting was set, when the user is not a donator */

/* Update Complete */
#define GOOD 5  /* Update good and successful, IP changed */
#define NOCHG 6 /* No changes, update considered abusive */

/* Hostname-Related Errors */
#define NOTFQDN 7 /* A Fully-Qualified Domain Name was not provided */
#define NOHOST 8  /* The Hostname specified does not exist */
#define NOTYOURS \
  9 /* The Hostname specified exists, but not under the username specified */
#define NUMHOST 10 /* Too many or too few hosts found */
#define ABUSE 11   /* The hostname specified is blocked for update abuse */

/* Server Error Conditions */
#define DNSERR 12 /* DNS error encountered */
#define NOO \
  13 /* Short write for 911, there is a serious problem on dyndns side. */

class DynDDNS : public DDNS {
 public:
  DynDDNS(EventLoop *loop, const InetAddress &addr) : DDNS(loop, addr) {}
  ~DynDDNS() = default;

  void FastRPCSuccCallback(const ConnectionPtr &conn) {}
  void FastRPCReadCallback(const ConnectionPtr &conn) {}
  void FastRPCWriteCallback(const ConnectionPtr &conn) {}
  void FastRPCCloseCallback(const ConnectionPtr &conn) {}

  bool GetIPAddress() override {
    // "CurrentIPAddress:"
    return true;
  }
  bool IPCheck() override {
    ctor_.reset(new Connector(loop_, server_addr_));
    ctor_->Connect();

    auto conn = ctor_->conn();
    conn->SetConnSuccCb(std::bind(&DynDDNS::FastRPCSuccCallback, this, conn));
    conn->SetConnReadCb(std::bind(&DynDDNS::FastRPCReadCallback, this, conn));
    conn->SetConnWriteCb(std::bind(&DynDDNS::FastRPCWriteCallback, this, conn));
    conn->SetConnCloseCb(std::bind(&DynDDNS::FastRPCCloseCallback, this, conn));
    conn->AddGeneralEvent();

    std::string check_ip_reqest = "GET / HTTP/1.1\r\n" + "Host: " +
                                  server_addr_.host() + "\r\n" +
                                  "User-Agent: " + user_agent_ + "\r\n\r\n";

    char *buffer =
        static_cast<char *>(conn->Malloc(check_ip_reqest.size(), 64));

    conn->SubmitWriteBufferSafe(buffer, check_ip_reqest.size());
    conn->AddWriteEvent();
    return true;
  }

  /* needed? check if local ip changed */
  bool IsLocalIPChanged() { return true; }

  bool IsOutboudIPChanged() { return true; }

  bool ParseResponse() {
    std::string line;

    static std::map<std::string, int> kRetcodeMap = {
        // DYNDDNS MSG: system parameter is invalid
        {"badsys", BADSYS},
        // useragent that was sent has beed blocked
        {"badagent", BADAGENT},
        // username or password are incorrect
        {"badauth", BADAUTH},
        // an option available only to credited users
        {"!donator", NOTDONATOR},
        // update complete good
        {"good", GOOD},
        // IP not changed
        {"nochg", NOCHG},
        // host name specified is not FQDN.
        {"notfqdn", NOTFQDN},
        // hostname not exist
        {"nohost", NOHOST},
        // hostname not belong to current user
        {"!yours", NOTYOURS},
        // too many or too few hosts found
        {"numhost", NUMHOST},
        // host name blocked for update abuse
        {"abuse", ABUSE},
        // DNS error encountered. better sotp dyndns
        {"dnserr", DNSERR},
        // serious problem on Dyndns side. better stop dyndns
        {"911", NOO},
        {"!yours", NOTYOURS},
        {"!yours", NOTYOURS}};

    return true;
  }

  bool Update() {
    DDNSInfo info;
    info.hostname_ = domain_;
    info.wildcard_ = (wildcard_ == 1 ? "ON" : "OFF");
    info.mx_ = "NO";
    info.backmx_ = "NO";
    info.offline_ = (offline_ == 1 ? "YES" : "NO");

    std::string update_url;
    if (system_ == DYNDNS) {
      update_url = "/nic/update?system=dyndns&hostname=" + info.hostname_ +
                   "&myip=" + info.myip_ + "&wildcard=" + info.wildcard_ +
                   "&mx=" + info.mx_ + "&backmx=" + info.backmx_ + "&offline=" +
                   info.offline_;
    } else if (system_ == STATDNS) {
      update_url = "/nic/update?system=statdns&hostname=" + info.hostname_ +
                   "&myip=" + info.myip_ + "&wildcard=" + info.wildcard_ +
                   "&mx=" + info.mx_ + "&backmx=" + info.backmx_ + "&offline=" +
                   info.offline_;
    } else if (system_ == CUSTOM) {
      update_url = "/nic/update?system=custom&hostname=" + info.hostname_ +
                   "&myip=" + info.myip_ + "&wildcard=" + info.wildcard_ +
                   "&mx=" + info.mx_ + "&backmx=" + info.backmx_ + "&offline=" +
                   info.offline_;
    } else {
      return false;
    }

    std::string user_pass = user_name_ + ":" + pass_word_;
    std::string user_pass_encode =
        base64_encode(user_pass.c_str(), user_pass.size());

    std::string get_url = "GET " + update_url + " HTTP/1.1\r\n" + "Host: " +
                          kDefaultDynDDNSUpdate + "\r\n" +
                          "Authorization: Basic " + user_pass_encode + "\r\n" +
                          "User-Agent: " + user_agent_ + "\r\n\r\n";

    // members.dyndns.org 80
    ParseResponse();
    // if(ret == GOOD || ret == NOCHG) ok
    return true;
  }

  bool Start() override {
    bool force_update = false;
    /* check if outbound ip changed */
    if (IPCheck()) {
      /* parse received buffer */
      force_update = GetIPAddress();
    } else {
      force_update = IsLocalIPChanged();
    }
    if (force_update) {
      Update();
    }

    switch (state_) {
      case kDDNStateGetIPRequest:

        break;
      case kDDNStateGetIPResponse:

      default:
        break;
    }

    return true;
  }
};

#define NOIP_NAME "dynupdate.no-ip.com"
static const std::string kNoIPScript = "ip1.dynupdate.no-ip.com";
static const std::string kNoIPVersion = "2.1.7";
static const std::string kNoIPUserAgent = "User-Agent: Linux-DUC/";
#define SETTING_SCRIPT "settings.php?"
#define USTRNG "username="
#define PWDSTRNG "&pass="

static const std::string kNoIPUpdateScript = "ducupdate.php";

#define ALREADYSET 0
#define SUCCESS 1
#define BADHOST 2
#define BADPASSWD 3
#define BADUSER 4
#define TOSVIOLATE 6
#define PRIVATEIP 7
#define HOSTDISABLED 8
#define HOSTISREDIRECT 9
#define BADGRP 10
#define SUCCESSGRP 11
#define ALREADYSETGRP 12
#define RELEASEDISABLED 99

#define UNKNOWNERR (-1)
#define FATALERR (-1)
#define NOHOSTLOOKUP (-2)
#define SOCKETFAIL (-3)
#define CONNTIMEOUT (-4)
#define CONNFAIL (-5)
#define READTIMEOUT (-6)
#define READFAIL (-7)
#define WRITETIMEOUT (-8)
#define WRITEFAIL (-9)

class NoIPDDNS : public DDNS {
 public:
  NoIPDDNS(EventLoop *loop, const InetAddress &addr) : DDNS(loop, addr) {}
  ~NoIPDDNS() = default;

  void FastRPCSuccCallback(const ConnectionPtr &conn) {}
  void FastRPCReadCallback(const ConnectionPtr &conn) {}
  void FastRPCWriteCallback(const ConnectionPtr &conn) {}
  void FastRPCCloseCallback(const ConnectionPtr &conn) {}

 private:
  std::string server_name_ = NOIP_NAME;
  uint16_t port_;
  uint32_t new_bonud_ip_;

  char *user = "test";
  char *pwd = "test";
  char *domain = "test.no-ip.com";

  int handle_dynup_error(int error_code) {
    char *err_string = nullptr;
    char *ourname = "domain";
    char *IPaddress = "newip";
    char *login = nullptr;

    if (error_code == SUCCESS || error_code == SUCCESSGRP) {
      // LOG(INFO) << "%s set to %s", ourname, IPaddress);
      return SUCCESS;
    }

    err_string = strerror(errno);
    switch (error_code) {
      case ALREADYSET:
      case ALREADYSETGRP:
        // LOG(INFO) << "%s was already set to %s.", ourname, IPaddress);
        return SUCCESS;
      case BADHOST:
        // LOG(INFO) << "No host '%s' found at %s.", ourname, NOIP_NAME);
        break;
      case BADPASSWD:
        // LOG(INFO) << "Incorrect password for %s while attempting toupdate
        // %s.",login, ourname);
        break;
      case BADUSER:
        // LOG(INFO) << "No user '%s' found at %s.", login, NOIP_NAME);
        break;
      case PRIVATEIP:
        // LOG(INFO) << "IP address '%s' is a private network address.",
        // IPaddress);
        // LOG(INFO) << "Use the NAT facility.");
        break;
      case TOSVIOLATE:
        // LOG(INFO) << "Account '%s' has been banned for violating terms of
        // service.",login);
        break;
      case HOSTDISABLED:
        // LOG(INFO) << "Host '%s' has been disabled and cannot be updated.",
        // ourname);
        break;
      case HOSTISREDIRECT:
        // LOG(INFO) << "Host '%s' not updated because it is a web redirect.",
        // ourname);
        break;
      case BADGRP:
        // LOG(INFO) << "No group '%s' found at %s.", ourname, NOIP_NAME);
        break;
      /*
      case RELEASEDISABLED:
              // LOG(INFO) << CMSG99);
              // LOG(INFO) << CMSG99a);
              //strcpy(shared->banned_version, VERSION);
              break;
      */
      case UNKNOWNERR:
        // LOG(INFO) << "Unknown error trying to set %s at %s.", ourname,
        // NOIP_NAME);
        return FATALERR;
      case NOHOSTLOOKUP:
        // LOG(INFO) << "Can't gethostbyname for %s", NOIP_NAME);
        break;
      case SOCKETFAIL:
        // LOG(INFO) << "Can't create socket (%s)", err_string);
        break;
      case CONNTIMEOUT:
        // LOG(INFO) << "Connect to %s timed out", NOIP_NAME);
        break;
      case CONNFAIL:
        // LOG(INFO) << "Can't connect to %s (%s)", NOIP_NAME, err_string);
        break;
      case READTIMEOUT:
        // LOG(INFO) << "Read timed out talking to %s", NOIP_NAME);
        break;
      case READFAIL:
        // LOG(INFO) << "Read from %s failed (%s)", NOIP_NAME, err_string);
        break;
      case WRITETIMEOUT:
        // LOG(INFO) << "Write timed out talking to %s", NOIP_NAME);
        break;
      case WRITEFAIL:
        // LOG(INFO) << "Write to %s failed (%s)", NOIP_NAME, err_string);
        break;
      default:
        // LOG(INFO) << "Unknown error %d trying to set %s at %s.",
                        error_code, ourname, NOIP_NAME);
                        return FATALERR;
    }

    return UNKNOWNERR;
  }

  uint32_t GetOutboundIP() {
    ctor_.reset(new Connector(loop_, server_addr_));
    ctor_->Connect();

    auto conn = ctor_->conn();
    conn->SetConnSuccCb(std::bind(&NoIPDDNS::FastRPCSuccCallback, this, conn));
    conn->SetConnReadCb(std::bind(&NoIPDDNS::FastRPCReadCallback, this, conn));
    conn->SetConnWriteCb(
        std::bind(&NoIPDDNS::FastRPCWriteCallback, this, conn));
    conn->SetConnCloseCb(
        std::bind(&NoIPDDNS::FastRPCCloseCallback, this, conn));
    conn->AddGeneralEvent();

    std::string request_url = "GET http://" + kNoIPScript + "/ HTTP/1.0\r\n" +
                              kNoIPUserAgent + kNoIPVersion + "\r\n\r\n";

    char *buffer = static_cast<char *>(conn->Malloc(request_url.size(), 64));

    conn->SubmitWriteBufferSafe(buffer, request_url.size());
    conn->AddWriteEvent();

    // recv buffer
    // 解析ip地址
    // new_bonud_ip_
    return true;
  }
  /* needed? check if local ip changed */
  bool IsLocalIPChanged() { return true; }

  bool IsOutboudIPChanged() { return true; }

  bool Update() {
    struct in_addr ipaddr;
    ipaddr.s_addr = new_bonud_ip_;

    // set up text for web page updatej
    std::string body = "username=" + user_name_ + "&pass=" + pass_word_ +
                       "&h[]=" + domain_ + "&ip=" + ::inet_ntoa(ipaddr) + " ";
    std::string body_buffer = base64_encode(body.c_str(), body.size());

    std::string buffer = "GET http://" + server_name_ + "/" +
                         kNoIPUpdateScript + "?" + "requestL=" + body_buffer +
                         " HTTP/1.0\r\n" + kNoIPUserAgent + kNoIPVersion +
                         "\r\n\r\n";

    // send

    //
    return true;
  }
  bool Start() override {
    bool force_update = false;
    /* check if outbound ip changed */
    if (IPCheck()) {
      /* parse received buffer */
      force_update = GetIPAddress();
    } else {
      force_update = IsLocalIPChanged();
    }
    if (force_update) {
      Update();
    }
    return true;
  }
};

}  // namespace MSF
#endif