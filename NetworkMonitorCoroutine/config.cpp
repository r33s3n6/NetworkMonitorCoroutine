#include "config.h"
#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>
using namespace std;

#include "common_functions.h"
using namespace common;
void config::load_config_from_file(string path)//TODO:可以使用其他库重构代码
{
	ifstream conf_file;
	conf_file.open(path, ios::in);
	if (!conf_file) {
		save_config(path);
		conf_file.open(path, ios::in);
	}
	string line;
	while (getline(conf_file,line)) {
		if (line[0] == '#')
			continue;
		size_t pos = line.find("=");
		if (pos == string::npos)
			continue;
		string&& key = string_trim(line.substr(0, pos));
		conf_entry& entry=conf_entry_map[key];
		
		string&& value = string_trim(line.substr(pos + 1, line.size() - pos - 1));
		size_t array_pos = entry.type.find("[");
		try {
			if (entry.type == typeid(bool).name()) {
				if (value[0] == 'T' || value[0] == 't') {
					*(bool*)entry.address = true;
				}
				else {
					*(bool*)entry.address = false;
				}
			}
			else if (entry.type == typeid(size_t).name()) {
				*(size_t*)entry.address = boost::lexical_cast<size_t>(value);
			}
			else if (entry.type == typeid(string).name()) {
				*(string*)entry.address = value;
			}
			else if (array_pos != string::npos) {
				int* arr = (int*)entry.address;
				size_t pos2 = entry.type.find("]");
				if (pos2 == string::npos)
					continue;
				size_t s = boost::lexical_cast<int>(entry.type.substr(array_pos + 1, pos2 - array_pos - 1));
				shared_ptr<vector<string>> value_vec_ptr = string_split(value, ",");
				for (int i = 0; i < s && i < value_vec_ptr->size(); i++) {
					arr[i] = boost::lexical_cast<int>(string_trim((*value_vec_ptr)[i]));
				}

			}
		}
		catch (const std::exception& e)
		{
			cout << "ERROR when read config file. DEBUG_info: "<< key << endl;
			cout << e.what() << endl;
			continue;
		}
	}
	

}

void config::save_config(string path)
{
	ofstream conf_file(path);
	for (auto map_entry:conf_entry_map) {
		conf_file << map_entry.first << "=";

		conf_entry& entry = map_entry.second;
		size_t array_pos = entry.type.find("[");
		try
		{
			if (entry.type == typeid(bool).name()) {
				if (*(bool*)entry.address) {
					conf_file << "true";
				}
				else {
					conf_file << "false";
				}
			}
			else if (entry.type == typeid(size_t).name()) {
				conf_file << *(size_t*)entry.address;
			}
			else if (entry.type == typeid(string).name()) {
				conf_file << *(string*)entry.address;
			}
			else if (array_pos != string::npos) {
				int* arr = (int*)entry.address;
				size_t pos2 = entry.type.find("]");
				if (pos2 == string::npos)
					continue;
				size_t s = boost::lexical_cast<int>(entry.type.substr(array_pos + 1, pos2 - array_pos - 1));

				for (int i = 0; i < s; i++) {
					conf_file << arr[i] << ",";
				}

			}
			conf_file << endl;
		}
		catch (const std::exception& e)
		{
			cout << "ERROR when write config file" << endl;
			cout << e.what() << endl;
			continue;
		}
		
	}
	conf_file.close();
}


