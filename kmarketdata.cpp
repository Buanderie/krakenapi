#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>

#include "kapi.hpp"
#include "libjson/libjson.h"

using namespace std;
using namespace Kraken;

//------------------------------------------------------------------------------
// deals with Trades:
struct Trade {
   double price, volume; 
   time_t time; 
   char order;
};

//------------------------------------------------------------------------------
// prints a Trade
ostream& operator<<(ostream& os, const Trade& t) 
{
   struct tm timeinfo;
   gmtime_r(&t.time, &timeinfo);
   
   char buffer[20];
   strftime(buffer, 20, "%T", &timeinfo);

   return os << buffer << ','
	     << t.order << ','
	     << fixed
	     << setprecision(5) << t.price << ','
	     << setprecision(9) << t.volume;
}

//------------------------------------------------------------------------------
// helper function to load a Trade from a JSONNode:
Trade get_trade(const JSONNode& node) 
{
   Trade t; 
   t.price  = node[0].as_float();
   t.volume = node[1].as_float();
   t.time   = node[2].as_int();
   t.order  = node[3].as_string()[0];
   return t;
}

//------------------------------------------------------------------------------

int main() 
{ 
   curl_global_init(CURL_GLOBAL_ALL);

   try {
      KAPI kapi;
      KAPI::Input in;

      // get recent trades 
      in.insert(make_pair("pair", "XLTCZEUR"));

      string json_trades = kapi.public_method("Trades", in); 
      JSONNode root = libjson::parse(json_trades);

      // check for errors, if they're present an 
      // exception will be thrown
      if (!root.at("error").empty()) {
	 ostringstream oss;
	 oss << "Kraken response contains errors: ";

	 // append errors to output string stream
	 for (JSONNode::iterator it = root["error"].begin(); 
	      it != root["error"].end(); ++it) 
	    oss << endl << " * " << libjson::to_std_string(it->as_string());
	 
	 throw runtime_error(oss.str());
      }
      else {
	 // format the output in columns: time, order, price and volume
	 JSONNode result = root.at("result")[0];
	 
	 time_t step = 3600;
	 map<time_t,vector<Trade> > trades;

	 // group results by base time
	 for (JSONNode::const_iterator it = result.begin();
	      it != result.end(); ++it) {
	    Trade t = get_trade(*it);
	    time_t base = t.time - (t.time % step);
	    trades[base].push_back(t);
	 }
	
	 // create candlesticks
	 for (map<time_t,vector<Trade> >::const_iterator it = trades.begin();
	      it != trades.end(); ++it) 
	 {
	    cout << it->first << endl;
	 }

      }

   }
   catch(exception& e) {
      cerr << "Error: " << e.what() << endl;
   }
   catch(...) {
      cerr << "Unknow exception." << endl;
   }

   curl_global_cleanup();
   return 0;
}