#ifndef UTILS_H_
#define UTILS_H_
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

vector<string> split(const string& s,char x);
bool isValidParamVal(const string& str);
bool isalphanum(const string& str);
bool isValidUrlPart(const string& str);
vector<string> split(string s,const string& x);
int hexdigitToDecimal(char ch);
string url_decode(const string& s);
std::string lowercase(const std::string&);

#endif