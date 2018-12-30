
#pragma once

#include <string>
#include <utility>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;
using bpo::value;

namespace utils { namespace program_options {

	/*
	 * defines a basis for parsing program options automatically
	 */
	class options {

		bpo::options_description _options;
		bpo::positional_options_description _positional_options;
		bpo::variables_map vm;

	public:
		/*
		 * constructors
		 */

		// default constructor
		options();

		// constructor receiving a custom options description
		options(const char* desc);

		/*
		 * adding options
		 */

		// Adds a flag, which default to false
		// name should be a string like "level" or "level,l"
		// meaning that --help and -h will be used to identify this option
		options& flag(
				const char *name,		// flag name. can be of the form "name" (used via --name), or "name,n" (used via --name or the short-form -n)
				bool& var,				// reference to where to store the value
				const char *desc = "");	// optional description


		// Adds an options with the type being given by the template parameter
		//   options.add<int>("name", my_int, 2, "description")
		//
		// note that instead of an address, my_int is passed as a C++ reference
		template<class T>
		options& value(
				const char* name,		// flag name. can be of the form "name" (used via --name), or "name,n" (used via --name or the short-form -n)
				T& addr,				// reference to where to store the value
				T default_value = T(),	// default value
				const char *desc = "");	// optional description


		// This method should be called prior to reading any value
		void parse(int argc, char **argv);

		// Returns the amount of ocurrences for a given key
		int count(const char* name) const;

		// Indicates if a given key is present
		bool has(const char *name) const;

		// Gets the value of a flag
		bool get_flag(const char* name) const;

		// Gets the value of a given key
		template<class T> T get(const char* name) const;

		// Returns the help message (assumes option "help,h" exists)
		bpo::options_description help() const;

	private:
		// Initialization method
		// Adds the "help,h" option by default
		void _init();

		// Internal helper method to add a value option
		template<class T> options& _value(const char* name, bpo::value_semantic* semantic, const char *desc = NULL);

		// looks for a @filename option in the arguments
//		static std::pair<std::string, std::string> file_option_parser(const std::string& s);

		// load an options file
		void load_options_file(const std::string& filename);
	};

	/*
	 * template definitions
	 */

	template<class T>
	T options::get(const char* name) const {
		return this->vm[name].as<T>();
	}

	template<class T>
	options& options::value(const char* name, T& addr, T default_value, const char *desc) {
		return this->_value<T>(name, bpo::value<T>(&addr)->default_value(default_value), desc);
	}

	template<class T>
	options& options::_value(const char* name, bpo::value_semantic* semantic, const char *desc) {
		this->_options.add_options() (name, semantic, desc);
		return *this;
	}
} }