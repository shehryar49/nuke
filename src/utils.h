#ifndef UTILS_H_
#define UTILS_H_
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

vector<string> split(const string& s,char x)
{
  vector<string> parts;
  size_t len = s.length();
  size_t start = 0;
  for(size_t i = 0;i<len;i++)
  {
    if(s[i] == x)
    {
      //copy part s.substr(start,i-start)
      parts.push_back(s.substr(start,i - start));
      start = i+1;
    }
  }
  parts.push_back(s.substr(start,len - start));
  return parts;
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
bool isalphanum(const string& str)
{
  for(auto ch: str)
  {
    if(!isalpha(ch) && !isdigit(ch))
      return false;
  }
  return true;
}
bool isValidUrlPart(const string& str)
{
  for(auto ch: str)
  {
    if(!isalpha(ch) && !isdigit(ch) && ch!='_' && ch!='-')
      return false;
  }
  return true;
}
vector<string> split(string s,const string& x)
{
	size_t  k = 0;
	vector<string> list;
	while( (k = s.find(x) ) != std::string::npos)
	{
		list.push_back(s.substr(0,k));
		s = s.substr(k+x.length());
	}
	list.push_back(s);
	return list;
}
int hexdigitToDecimal(char ch)
{
  if(ch >= 'A' && ch <='F')
    return ch - 'A' + 10;
  else if(ch >= 'a' && ch <='f')
    return ch - 'a' + 10;
  else if(ch >= '0' && ch <= '9')
    return ch - '0';
  return 69;
}
string url_decode(const string& s)
{
    string res = "";
    int k = 0;
    size_t len = s.length();
    while(k<len)
    {
        if(s[k]=='%' && k+2 < len)
        {
            k+=2;
            res += (char)( hexdigitToDecimal(s[k+1])*16 + hexdigitToDecimal(s[k+2]) );
        }
        else if(s[k]=='+')
            res+=" ";
        else
          res+=s[k];
        k+=1;
    }
    return res;
}
#endif