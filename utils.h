#ifndef UTILS_H_
#define UTILS_H_
#include <string>
#include <vector>

using namespace std;

vector<string> split(string s,const string& x)
{
  unsigned int  k = 0;
  vector<string> list;
  while(s.find(x)!=std::string::npos)
  {
    k = s.find(x);
    list.push_back(s.substr(0,k));
    s = s.substr(k+x.length());
  }
  list.push_back(s);
  return list;
}
bool isValidParamVal(const string& str)
{
  for(auto e: str)
  {
    if(!isdigit(e) && !isalpha(e))
      return false;
  }
  return true;
}
#endif