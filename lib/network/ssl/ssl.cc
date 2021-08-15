#include <openssl  //bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

#if 0

// 转换私钥编码格式
bool ConvertKeyFormat(char *oldKey, int oldKeyLen, int oldFormat,
                      char *newKeyFile, int newFormat) {
  EVP_PKEY *key = nullptr;
  BIO *biout = nullptr;
  int ret;

  if ((biout = BIO_new_file(newKeyFile, "w")) == nullptr) return false;

  key = LoadKey(oldKey, oldKeyLen, nullptr, oldFormat);
  if (key) {
    if (newFormat == FORMAT_DER) ret = i2d_PrivateKey_bio(biout, key);
    if (newFormat == FORMAT_PEM)
      ret = PEM_write_bio_PrivateKey(biout, key, nullptr, nullptr, 0, 0, nullptr);
  } else
    return false;

  BIO_free(biout);
  EVP_PKEY_free(key);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 根据公私钥生成P12格式证书

bool CreateP12Cert(char *pubCertFile, char *priCertFile, int oldFormat,
                   char *p12CertFile, char *p12pass) {
  EVP_PKEY *key = nullptr;
  X509 *cert = nullptr;
  PKCS12 *p12;

  cert = LoadCert(pubCertFile, 0, nullptr, oldFormat);
  key = LoadKey(priCertFile, 0, nullptr, oldFormat);

  SSLeay_add_all_algorithms();
  p12 = PKCS12_create(p12pass, "(SecPass)", key, cert, nullptr, 0, 0, 0, 0, 0);
  if (!p12) {
    X509_free(cert);
    EVP_PKEY_free(key);
    return false;
  }

  FILE *fp = fopen(p12CertFile, "wb");
  i2d_PKCS12_fp(fp, p12);
  PKCS12_free(p12);
  fclose(fp);

  X509_free(cert);
  EVP_PKEY_free(key);
  EVP_cleanup();
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 解析P12格式证书为公钥和私钥

bool PraseP12Cert(char *p12Cert, char *p12pass, char *pubCertFile,
                  char *priCertFile, int format) {
  EVP_PKEY *key = nullptr;
  X509 *cert = nullptr;
  STACK_OF(X509) *ca = nullptr;
  BIO *bio = nullptr, *bioCert = nullptr, *bioKey = nullptr;
  PKCS12 *p12 = nullptr;
  int i, j;

  SSLeay_add_all_algorithms();
  bio = BIO_new_file(p12Cert, "r");
  p12 = d2i_PKCS12_bio(bio, nullptr);
  if (!PKCS12_parse(p12, p12pass, &key, &cert, &ca)) {
    BIO_free(bio);
    return false;
  }
  PKCS12_free(p12);

  bioCert = BIO_new_file(pubCertFile, "w");
  bioKey = BIO_new_file(priCertFile, "w");

  if (format == FORMAT_PEM) {
    i = PEM_write_bio_X509(bioCert, cert);
    j = PEM_write_bio_PrivateKey(bioKey, key, nullptr, nullptr, 0, 0, nullptr);
  }
  if (format == FORMAT_DER) {
    i = i2d_X509_bio(bioCert, cert);
    j = i2d_PrivateKey_bio(bioKey, key);
  }

  BIO_free(bio);
  BIO_free(bioCert);
  BIO_free(bioKey);
  X509_free(cert);
  EVP_PKEY_free(key);
  EVP_cleanup();
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 解析P12证书文件，结果存储在字符串对象中(PEM编码)

bool PraseP12CertToMem_PEM(char *p12Cert, char *p12pass, CString *pubCert,
                           CString *priCert) {
  EVP_PKEY *key = nullptr;
  X509 *cert = nullptr;
  STACK_OF(X509) *ca = nullptr;
  BIO *bio = nullptr, *bioCert = nullptr, *bioKey = nullptr;
  PKCS12 *p12 = nullptr;
  BUF_MEM *bptr;

  SSLeay_add_all_algorithms();
  bio = BIO_new_file(p12Cert, "r");
  p12 = d2i_PKCS12_bio(bio, nullptr);
  PKCS12_parse(p12, p12pass, &key, &cert, &ca);
  PKCS12_free(p12);

  bioKey = BIO_new(BIO_s_mem());
  BIO_set_close(bioKey, BIO_CLOSE);
  bioCert = BIO_new(BIO_s_mem());
  BIO_set_close(bioCert, BIO_CLOSE);

  char buf[4096];

  PEM_write_bio_X509(bioCert, cert);
  BIO_get_mem_ptr(bioCert, &bptr);
  memset(buf, 0, 4096);
  memcpy(buf, bptr->data, bptr->length);
  *pubCert = buf;

  PEM_write_bio_PrivateKey(bioKey, key, nullptr, nullptr, 0, 0, nullptr);
  BIO_get_mem_ptr(bioKey, &bptr);
  memset(buf, 0, 4096);
  memcpy(buf, bptr->data, bptr->length);
  *priCert = buf;

  BIO_free(bioCert);
  BIO_free(bioKey);
  X509_free(cert);
  EVP_PKEY_free(key);
  EVP_cleanup();
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 更改P12格式证书的密码

bool ChangeP12CertPassword(char *oldP12Cert, char *oldPass, char *newP12Cert,
                           char *newPass) {
  EVP_PKEY *key = nullptr;
  X509 *cert = nullptr;
  STACK_OF(X509) *ca = nullptr;
  BIO *bio = nullptr;
  PKCS12 *p12 = nullptr;

  SSLeay_add_all_algorithms();
  bio = BIO_new_file(oldP12Cert, "r");
  p12 = d2i_PKCS12_bio(bio, nullptr);
  if (!PKCS12_parse(p12, oldPass, &key, &cert, &ca)) {
    BIO_free(bio);
    return false;
  }
  PKCS12_free(p12);

  p12 = PKCS12_create(newPass, "(SecPass)", key, cert, nullptr, 0, 0, 0, 0, 0);
  if (!p12) {
    X509_free(cert);
    EVP_PKEY_free(key);
    return false;
  }

  FILE *fp = fopen(newP12Cert, "wb");
  i2d_PKCS12_fp(fp, p12);
  PKCS12_free(p12);
  fclose(fp);

  X509_free(cert);
  EVP_PKEY_free(key);
  EVP_cleanup();
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 根据证书请求文件签发证书

bool CreateCertFromRequestFile(char *rootPubCert, int rootPubCertLength,
                               char *rootPriCert, int rootPriCertLength,
                               int rootCertFormat, int serialNumber, int days,
                               char *requestFile, char *pubCert, char *priCert,
                               int format) {
  X509 *rootCert = nullptr;
  EVP_PKEY *rootKey = nullptr;
  int i, j;
  bool ret;

  OpenSSL_add_all_digests();

  rootKey = LoadKey(rootPriCert, rootPriCertLength, nullptr, rootCertFormat);
  rootCert = LoadCert(rootPubCert, rootPubCertLength, nullptr, rootCertFormat);

  if (rootKey == nullptr || rootCert == nullptr) return false;

  X509 *userCert = nullptr;
  EVP_PKEY *userKey = nullptr;
  X509_REQ *req = nullptr;
  BIO *in;
  in = BIO_new_file(requestFile, "r");
  req = PEM_read_bio_X509_REQ(in, nullptr, nullptr, nullptr);
  BIO_free(in);

  userKey = X509_REQ_get_pubkey(req);
  userCert = X509_new();

  X509_set_version(userCert, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(userCert), serialNumber);
  X509_gmtime_adj(X509_get_notBefore(userCert), 0);
  X509_gmtime_adj(X509_get_notAfter(userCert), (long)60 * 60 * 24 * days);
  X509_set_pubkey(userCert, userKey);
  EVP_PKEY_free(userKey);

  X509_set_subject_name(userCert, req->req_info->subject);

  X509_set_issuer_name(userCert, X509_get_issuer_name(rootCert));
  X509_sign(userCert, rootKey, EVP_sha1());

  BIO *bcert = nullptr, *bkey = nullptr;
  if (((bcert = BIO_new_file(pubCert, "w")) == nullptr) ||
      ((bkey = BIO_new_file(priCert, "w")) == nullptr))
    return false;

  if (format == FORMAT_DER) {
    ret = true;
    i = i2d_X509_bio(bcert, userCert);
    j = i2d_PrivateKey_bio(bkey, userKey);
  } else if (format == FORMAT_PEM) {
    ret = true;
    i = PEM_write_bio_X509(bcert, userCert);
    j = PEM_write_bio_PrivateKey(bkey, userKey, nullptr, nullptr, 0, nullptr, nullptr);
  }
  if (!i || !j) ret = false;

  BIO_free(bcert);
  BIO_free(bkey);
  X509_free(userCert);
  X509_free(rootCert);
  EVP_PKEY_free(rootKey);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 检查公私钥是否配对

bool CertPairCheck(char *pubCert, int pubCertLength, char *priCert,
                   int pricertLength, int format) {
  X509 *theCert = nullptr;
  EVP_PKEY *theKey = nullptr;

  theKey = LoadKey(priCert, pricertLength, nullptr, format);
  theCert = LoadCert(pubCert, pubCertLength, nullptr, format);

  bool ret;
  try {
    ret = X509_check_private_key(theCert, theKey);
  } catch (...) {
    ret = false;
  }
  X509_free(theCert);
  EVP_PKEY_free(theKey);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// 生成黑名单

bool CreateCrl(char *rootPubCert, int rootPubLen, char *rootPriCert,
               int rootPriLen, int rootCertFormat, LPCRLREQ crlInfo,
               int certNum, int hours, char *crlFile) {
  int ret = 1, i = 0;

  OpenSSL_add_all_algorithms();

  EVP_PKEY *pkey = LoadKey(rootPriCert, rootPriLen, nullptr, rootCertFormat);
  X509 *x509 = LoadCert(rootPubCert, rootPubLen, nullptr, rootCertFormat);

  const EVP_MD *dgst = EVP_get_digestbyname("sha1");

  X509_CRL *crl = X509_CRL_new();
  X509_CRL_INFO *ci = crl->crl;
  X509_NAME_free(ci->issuer);
  ci->issuer = X509_NAME_dup(x509->cert_info->subject);

  X509_gmtime_adj(ci->lastUpdate, 0);
  if (ci->nextUpdate == nullptr) ci->nextUpdate = ASN1_UTCTIME_new();
  X509_gmtime_adj(ci->nextUpdate, hours * 60 * 60);

  if (!ci->revoked) ci->revoked = sk_X509_REVOKED_new_null();

  X509_REVOKED *r = nullptr;
  BIGNUM *serial_bn = nullptr;
  char buf[512];

  for (i = 0; i < certNum; i++) {
    r = X509_REVOKED_new();
    ASN1_TIME_set(r->revocationDate, crlInfo[i].RevokeTime);
    BN_hex2bn(&serial_bn, ltoa(crlInfo[i].CertSerial, buf, 10));
    BN_to_ASN1_INTEGER(serial_bn, r->serialNumber);
    sk_X509_REVOKED_push(ci->revoked, r);
  }

  for (i = 0; i < sk_X509_REVOKED_num(ci->revoked); i++) {
    r = sk_X509_REVOKED_value(ci->revoked, i);
    r->sequence = i;
  }

  ci->version = ASN1_INTEGER_new();
  ASN1_INTEGER_set(ci->version, 1);
  X509_CRL_sign(crl, pkey, dgst);

  BIO *out = BIO_new(BIO_s_file());
  if (BIO_write_filename(out, crlFile) > 0) {
    PEM_write_bio_X509_CRL(out, crl);
  }
  X509V3_EXT_cleanup();

  BIO_free_all(out);
  EVP_PKEY_free(pkey);
  X509_CRL_free(crl);
  X509_free(x509);
  EVP_cleanup();
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// 通过黑名单验证证书，验证通过返回真，否则返回假

bool CheckCertWithCrl(char *pubCert, int pubCertLen, int certFormat,
                      char *crlData, int crlLen) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);

  BIO *in = nullptr;
  if (crlLen == 0) {
    if ((in = BIO_new_file(crlData, "r")) == nullptr) return nullptr;
  } else {
    if ((in = BIO_new_mem_buf(crlData, crlLen)) == nullptr) return nullptr;
  }
  X509_CRL *crl = PEM_read_bio_X509_CRL(in, nullptr, nullptr, nullptr);
  STACK_OF(X509_REVOKED) *revoked = crl->crl->revoked;
  X509_REVOKED *rc;

  ASN1_INTEGER *serial = X509_get_serialNumber(x509);
  int num = sk_X509_REVOKED_num(revoked);
  bool bf = true;
  for (int i = 0; i < num; i++) {
    rc = sk_X509_REVOKED_pop(revoked);
    if (ASN1_INTEGER_cmp(serial, rc->serialNumber) == 0) bf = false;
  }

  X509_CRL_free(crl);
  X509_free(x509);
  EVP_cleanup();
  return bf;
}

/////////////////////////////////////////////////////////////////////////////
// 通过根证书验证证书

bool CheckCertWithRoot(char *pubCert, int pubCertLen, int certFormat,
                       char *rootCert, int rootCertLen, int rootFormat) {
  OpenSSL_add_all_algorithms();
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  X509 *root = LoadCert(rootCert, rootCertLen, nullptr, rootFormat);

  EVP_PKEY *pcert = X509_get_pubkey(root);
  int ret = X509_verify(x509, pcert);
  EVP_PKEY_free(pcert);

  X509_free(x509);
  X509_free(root);
  if (ret == 1)
    return true;
  else
    return false;
}

/////////////////////////////////////////////////////////////////////////////
// 检查证书有效期,在有效期内返回真，否则返回假

bool CheckCertLife(char *pubCert, int pubCertLen, int certFormat, CTime time) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  time_t ct = time.GetTime();
  asn1_string_st *before = X509_get_notBefore(x509),
                 *after = X509_get_notAfter(x509);
  ASN1_UTCTIME *be = ASN1_STRING_dup(before), *af = ASN1_STRING_dup(after);
  bool bf;
  if (ASN1_UTCTIME_cmp_time_t(be, ct) >= 0 ||
      ASN1_UTCTIME_cmp_time_t(af, ct) <= 0)
    bf = false;
  else
    bf = true;
  M_ASN1_UTCTIME_free(be);
  M_ASN1_UTCTIME_free(af);
  X509_free(x509);
  return bf;
}

/////////////////////////////////////////////////////////////////////////////
// 从公钥证书和私钥证书中获取RSA密钥对信息，获取结果为PEM编码

void GetRSAKeyPairFromCertFile(char *pubCert, int pubCertLen, char *priCert,
                               int priCertLen, int certFormat,
                               LPRSAKEYPAIR rsa) {
  X509 *theCert = nullptr;
  EVP_PKEY *priKey = nullptr;
  BUF_MEM *bptr;
  priKey = LoadKey(priCert, priCertLen, nullptr, certFormat);
  theCert = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  EVP_PKEY *pubKey = X509_get_pubkey(theCert);
  rsa->Bits = EVP_PKEY_bits(pubKey);

  BIO *bcert = BIO_new(BIO_s_mem());
  BIO_set_close(bcert, BIO_CLOSE);
  PEM_write_bio_RSAPublicKey(bcert, pubKey->pkey.rsa);
  BIO_get_mem_ptr(bcert, &bptr);
  memcpy(rsa->PublicKey, bptr->data, bptr->length);
  BIO_free(bcert);

  BIO *bkey = BIO_new(BIO_s_mem());
  BIO_set_close(bcert, BIO_CLOSE);
  PEM_write_bio_PrivateKey(bkey, priKey, nullptr, nullptr, 0, nullptr, nullptr);
  BIO_get_mem_ptr(bcert, &bptr);
  memcpy(rsa->PrivateKey, bptr->data, bptr->length);
  BIO_free(bkey);

  X509_free(theCert);
  EVP_PKEY_free(priKey);
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的序列号

int GetCertSerialNumber(char *pubCert, int pubCertLen, int certFormat) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  char *stringval = i2s_ASN1_INTEGER(nullptr, X509_get_serialNumber(x509));
  X509_free(x509);
  return atoi(stringval);
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的颁发者名称（全部信息）

CString GetCertIssuer(char *pubCert, int pubCertLen, int certFormat) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  char buf[256];
  memset(buf, 0, 256);
  Get_Name(X509_get_issuer_name(x509), buf);
  X509_free(x509);
  CString str = buf;
  return str;
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的主题信息（全部信息），返回主题的字符串形式

CString GetCertSubjectString(char *pubCert, int pubCertLen, int certFormat) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  char buf[256];
  memset(buf, 0, 256);
  Get_Name(X509_get_subject_name(x509), buf);
  X509_free(x509);
  CString str = buf;
  return str;
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的签名算法

CString GetCertAlgorithm(char *pubCert, int pubCertLen, int certFormat) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  char buf[256];
  memset(buf, 0, 256);
  i2t_ASN1_OBJECT(buf, 1024, x509->cert_info->signature->algorithm);
  X509_free(x509);
  CString str = buf;
  return str;
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的有效期

void GetCertLife(char *pubCert, int pubCertLen, int certFormat,
                 CTime *notBefore, CTime *notAfter) {
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  asn1_string_st *before = X509_get_notBefore(x509),
                 *after = X509_get_notAfter(x509);
  ASN1_UTCTIME *be = ASN1_STRING_dup(before), *af = ASN1_STRING_dup(after);

  *notBefore = ConvertASN1Time(be);
  *notAfter = ConvertASN1Time(af);

  M_ASN1_UTCTIME_free(be);
  M_ASN1_UTCTIME_free(af);
  X509_free(x509);
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书的主题信息

int GetCertSubject(char *pubCert, int pubCertLen, int certFormat,
                   LPCERTSUBJECT subject) {
  X509_NAME_ENTRY *entry;
  ASN1_OBJECT *obj;
  ASN1_STRING *str;
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  X509_NAME *name = X509_get_subject_name(x509);
  int num = X509_NAME_entry_count(name);
  int fn_nid;
  for (int i = 0; i < num; i++) {
    entry = (X509_NAME_ENTRY *)X509_NAME_get_entry(name, i);
    obj = X509_NAME_ENTRY_get_object(entry);
    str = X509_NAME_ENTRY_get_data(entry);
    fn_nid = OBJ_obj2nid(obj);
    switch (fn_nid) {
      case NID_commonName:
        strcpy(subject->CN, LPCSTR(ConvterASN1String(str)));
      case NID_stateOrProvinceName:
        strcpy(subject->SP, LPCSTR(ConvterASN1String(str)));
      case NID_localityName:
        strcpy(subject->L, LPCSTR(ConvterASN1String(str)));
      case NID_organizationName:
        strcpy(subject->O, LPCSTR(ConvterASN1String(str)));
      case NID_organizationalUnitName:
        strcpy(subject->OU, LPCSTR(ConvterASN1String(str)));
      case NID_pkcs9_emailAddress:
        strcpy(subject->EMAIL, LPCSTR(ConvterASN1String(str)));
      case NID_email_protect:
        strcpy(subject->PMAIL, LPCSTR(ConvterASN1String(str)));
      case NID_title:
        strcpy(subject->T, LPCSTR(ConvterASN1String(str)));
      case NID_description:
        strcpy(subject->D, LPCSTR(ConvterASN1String(str)));
      case NID_givenName:
        strcpy(subject->G, LPCSTR(ConvterASN1String(str)));
    }
  }
  X509_free(x509);
  return num;
}

/////////////////////////////////////////////////////////////////////////////
// 获取证书扩展项目信息

int GetCertExtent(char *pubCert, int pubCertLen, int certFormat,
                  LPCERTEXT ext) {
  X509_EXTENSION *ex;
  ASN1_OBJECT *obj;
  int fn_nid;
  BIO *bio;
  BUF_MEM *bptr;
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, certFormat);
  STACK_OF(X509_EXTENSION) *exts = x509->cert_info->extensions;
  int count = sk_X509_EXTENSION_num(exts);
  for (int i = 0; i < count; i++) {
    ex = sk_X509_EXTENSION_value(exts, i);
    obj = X509_EXTENSION_get_object(ex);
    fn_nid = OBJ_obj2nid(obj);
    ext[i].IOID = fn_nid;
    bio = BIO_new(BIO_s_mem());
    BIO_set_close(bio, BIO_CLOSE);
    if (!X509V3_EXT_print(bio, ex, X509_FLAG_COMPAT, 1))
      M_ASN1_OCTET_STRING_print(bio, ex->value);
    BIO_get_mem_ptr(bio, &bptr);
    memcpy(ext[i].VALUE, bptr->data, bptr->length);
    CString str = ext[i].VALUE;
    if (str.Find("..") == 0) {
      str = str.Mid(2);
      strcpy(ext[i].VALUE, LPCSTR(str));
    }
    BIO_free(bio);
  }
  X509_free(x509);
  return count;
}

/////////////////////////////////////////////////////////////////////////////
// 数字签名

bool Sign(char *priCert, int priCertLen, int format, BYTE *input, long inputLen,
          BYTE *output, UINT *outputLen) {
  EVP_MD_CTX md_ctx;
  EVP_PKEY *priKey;
  OpenSSL_add_all_digests();
  priKey = LoadKey(priCert, priCertLen, nullptr, format);
  EVP_SignInit(&md_ctx, EVP_sha1());
  EVP_SignUpdate(&md_ctx, input, inputLen);
  int ret = EVP_SignFinal(&md_ctx, (BYTE *)output, outputLen, priKey);
  EVP_PKEY_free(priKey);
  if (ret == 1)
    return true;
  else
    return false;
}

/////////////////////////////////////////////////////////////////////////////
// 签名验证

bool Verify(char *pubCert, int pubCertLen, int format, BYTE *input,
            UINT inputLen, BYTE *sign, UINT signLen) {
  OpenSSL_add_all_digests();
  X509 *x509 = LoadCert(pubCert, pubCertLen, nullptr, format);
  EVP_PKEY *pcert = X509_get_pubkey(x509);
  EVP_MD_CTX md_ctx;
  EVP_VerifyInit(&md_ctx, EVP_sha1());
  EVP_VerifyUpdate(&md_ctx, input, inputLen);
  int ret = EVP_VerifyFinal(&md_ctx, sign, signLen, pcert);
  EVP_PKEY_free(pcert);
  X509_free(x509);
  if (ret == 1)
    return true;
  else
    return false;
}
#endif