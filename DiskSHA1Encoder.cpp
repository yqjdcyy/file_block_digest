
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string.h>

#include "DiskSHA1Encoder.h"
#include "file_block_digest.h"

using namespace std;

string Readfile(const string& file_path) {
  ifstream file(file_path.c_str());
  if (!file.is_open()) {
    return "";
  }
  int len;
  file.seekg(0, ios::end);
  len = file.tellg();
  file.seekg(0, ios::beg);
  if (len == 0) {
    file.close();
    return "";
  }
  char *buf = new char[len];
  file.read(buf, len);
  file.close();
  string content(buf, len);
  delete [] buf;
  return content;
}


jstring charTojstring(JNIEnv* env, const char* pat) {
    jclass strClass = (env)->FindClass("Ljava/lang/String;");
    jmethodID ctorID = (env)->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
    jbyteArray bytes = (env)->NewByteArray(strlen(pat));
    (env)->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*) pat);
    jstring encoding = (env)->NewStringUTF("GB2312");
    return (jstring) (env)->NewObject(strClass, ctorID, bytes, encoding);
}

char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

extern "C" {
JNIEXPORT jstring JNICALL Java_DiskSHA1Encoder_encode(JNIEnv *env, jobject obj, jstring path){

  string content = Readfile(jstringToChar(env, path));
  file_block_digest::FileDigestInfo upload_info;
  file_block_digest::GetFileDigestInfo(content.c_str(), content.size(), &upload_info);

  std::string response;
  for (size_t i = 0; i < upload_info.parts.size(); ++i) {
    file_block_digest::BlockInfo & part = upload_info.parts[i];
        response+=(part.cumulate_sha1+ ",");
  }

  return charTojstring(env, response.c_str());
}
}