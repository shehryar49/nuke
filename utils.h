#ifndef UTILS_H_
#define UTILS_H_
#include <string>
#include <vector>
#include <algorithm>
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
int to_decimal(string HEX)
{
    int ans = 0;
    std::reverse(HEX.begin(),HEX.end());
    int x = 1;
    for(auto e: HEX)
    {
        if(e>='0' && e<='9')
          ans+= (e-48)*x;
        else if(isalpha(e))
        {
          e = toupper(e);
          ans+=(e-55)*x;
        }
        x*=16;
    }
    return ans;
}
string url_decode(const string& s)
{
    string res = "";
    int k = 0;
    while(k<s.length())
    {
        if(s[k]=='%')
        {
            //string HEX;
          //  HEX+=s[k+1];
          //  HEX+=s[k+2];
            char a = s[k+1];
            char b = s[k+2];
            k+=2;
            if(isalpha(a))
            {
              toupper(a);
              a-=55;
            }
            else
            {
              a-=48;
            }
            if(isalpha(b))
            {
              toupper(b);
              b-=55;
            }
            else
            {
              b-=48;
            }
            res += (char)(a*16 + b);
        }
        else if(s[k]=='+')
        {
            res+=" ";
        }
        else
        {
            res+=s[k];
        }
        k+=1;
    }
    return res;
}
#endif