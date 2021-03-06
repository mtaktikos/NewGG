#include "chess.h"


////
//// Includes
////

 #include <algorithm>
#include <sstream>

#include "misc.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"














using std::string;

UCI::OptionsMap Options; // Global object

namespace UCI {

	/// 'On change' actions, triggered by an option's value change
	void on_clear_hash(const Option&) { Search::clear(); }
	void on_hash_size(const Option& o) { TT.resize(o); }
	void on_logger(const Option& o) { start_logger(o); }
	void on_eval(const Option&) { Eval::init(); }
	void on_threads(const Option&) { Threads.read_uci_options(); }
	
	
	/// Our case insensitive less() function as required by UCI protocol
	bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
			[](char c1, char c2) { return tolower(c1) < tolower(c2); });
	}

	
/// init() initializes the UCI options to their hard coded default values

void init(OptionsMap& o) {

	const int MaxHashMB =1024 * 1024 ; // Is64Bit ? 1024 * 1024 : 2048;

	o["Write Debug Log"] << Option(false, on_logger);
	o["Contempt"] << Option(0, -100, 100);
	o["Threads"] << Option(5, 1, MAX_CPUS, on_threads);
	o["Hash"] << Option(512, 1, MaxHashMB, on_hash_size);
	o["Clear Hash"] << Option(on_clear_hash);
	o["Ponder"] << Option(true);
	o["MultiPV"] << Option(1, 1, 500);
	o["Skill Level"] << Option(20, 0, 20);
	o["Move Overhead"] << Option(20, 0, 5000);
	o["Minimum Thinking Time"] << Option(20, 0, 5000);
	o["Slow Mover"] << Option(89, 10, 1000);
	o["nodestime"] << Option(0, 0, 10000);
	o["UseOpenNet"] << Option(false);
	//o["UCI_Chess960"] << Option(false);
}






/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

	for (size_t idx = 0; idx < om.size(); ++idx)
		for (const auto& it : om)
			if (it.second.idx == idx)
			{
				const Option& o = it.second;
				os << "\noption name " << it.first << " type " << o.type;

				if (o.type != "button")
					os << " default " << o.defaultValue;

				if (o.type == "spin")
					os << " min " << o.min << " max " << o.max;

				break;
			}

	return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{
	defaultValue = currentValue = v;
}

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{
	defaultValue = currentValue = (v ? "true" : "false");
}

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(int v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{
	defaultValue = currentValue = std::to_string(v);
}

Option::operator int() const {
	assert(type == "check" || type == "spin");
	return (type == "spin" ? stoi(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
	assert(type == "string");
	return currentValue;
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

	static size_t insert_order = 0;

	*this = o;
	idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

	assert(!type.empty());

	if ((type != "button" && v.empty())
		|| (type == "check" && v != "true" && v != "false")
		|| (type == "spin" && (stoi(v) < min || stoi(v) > max)))
		return *this;

	if (type != "button")
		currentValue = v;

	if (on_change)
		on_change(*this);

	return *this;
}



} // namespace UCI

