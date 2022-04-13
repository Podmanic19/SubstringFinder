#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <optional>

const short MAX_SUBSTRING_LENGTH    =  256;
const short MAX_SUPPORTED_CHARACTER =  127;
const short MAX_LOADED_CHARS_NUM    = 1024;

struct Substring {

	short matchesUpToNow = 0;
	std::string string;
	std::vector<short> table;

};

struct TextMeta {

	uint64_t row = 0;
	uint64_t col = 0;
	std::string stringBuff;
	std::vector<uint64_t> row_length;

};

struct FoundSubstringsKeeper {

	std::vector<std::pair<uint64_t, uint64_t>>   positions;							//positions where the substring was found and a pair for it was also found
	std::optional<std::pair<uint64_t, uint64_t>> unpairedSubstring;					//position of substring for which no pair was found yet

};

std::ifstream fileLoading(const std::string& path) {

	std::ifstream input;
	input.open(path);

	if (input.fail()) {
		throw "Failed to open specified file.";
	}

	return input;

}

std::string substringLoading(const std::string& substring) {

	if (substring.size() >= MAX_SUBSTRING_LENGTH || substring.size() <= 0) {
		throw "Invalid substring length - only substring shorter than 256 and longer than 0 characters are accepted";
	}

	return substring;

}

bool is_digits(const std::string& str) {
	return str.find_first_not_of("0123456789") == std::string::npos;
}

uint32_t distanceLoading(std::string distanceString) {

	distanceString.erase(std::remove_if(distanceString.begin(), distanceString.end(), ::isspace), distanceString.end()); // getting rid of empty spaces

	if (!is_digits(distanceString)) {																					// check if string consists of only numbers
		throw "Invalid distance between substrings parameter, please enter a positive number between 0 and 4 294 967 295.";
	}

	uint32_t distance = strtoul(distanceString.c_str(), NULL, 10);

	if (errno == ERANGE || distance == 0 || distanceString[0] == '-') {
		throw "Invalid distance between substrings parameter, please enter a positive number between 0 and 4 294 967 295.";
	}

	return distance;

}

std::vector<short> buildTable(std::string string) {				//building table for KMP algorithm
	
	std::vector<short> table;
	size_t length = string.size();
	short index = 0;

	table.resize(length);
	table[0] = 0;												// set the initial suffix that == prefix to the first character

	for (size_t i = 1; i < length;) {
		if (string[i] == string[index]) {
			table[i] = index + 1;
			index++;
			i++;
		}
		else {
			if (index != 0) {
				index = table[index - 1];
			}
			else {
				table[i] = 0;
				i++;
			}
		}
	}

	return table;

}

uint64_t distanceFromPrev(TextMeta& text, std::pair<uint64_t, uint64_t> positionPrev, std::pair<uint64_t, uint64_t> position) {

	uint64_t distance = 0;

	uint64_t rowPrev = positionPrev.first;
	uint64_t colPrev = positionPrev.second + 1;

	uint64_t rowCurr = position.first;
	uint64_t colCurr = position.second + 1;							//+1 converts from index to length

	while (rowCurr != rowPrev) {
		distance += colCurr;
		colCurr = text.row_length[--rowCurr];
	}

	distance += colCurr - colPrev;

	return distance;

}

void pairSubstrings(TextMeta text, FoundSubstringsKeeper& solution, std::pair<uint64_t, uint64_t> position, uint32_t distance) {

	if (solution.unpairedSubstring.has_value()) {												//if there is an unpaired solution
		if (distance >= distanceFromPrev(text, solution.unpairedSubstring.value(), position)){	//and it is within distance
			solution.positions.push_back(solution.unpairedSubstring.value());
			solution.positions.push_back(position);												//add both positions
			solution.unpairedSubstring.reset();
			return;
		}
	}
	else {
		if (!solution.positions.empty() && distance >= distanceFromPrev(text, solution.positions.back(), position)) {	//if this is the first found substring
			solution.positions.push_back(position);
			return;
		}
	}

	solution.unpairedSubstring = position;												//if the previous one is too far, set this one as the unpaired one

}

std::pair<uint64_t, uint64_t> calculatePosition(TextMeta text, Substring substring) {

	uint64_t row = text.row;
	uint64_t column = text.col + 1;
	uint64_t remainder = substring.matchesUpToNow - 1;
	std::pair<uint64_t, uint64_t> position;

	while (remainder > column) {											//while the substring cant fit into this row
		remainder = remainder - column;										//how many matches there are outside of this row
		column = text.row_length[--row];									//set column to the last column in previous row
	}

	position.first = row;
	position.second = column - remainder - 1;

	return position;

}


void KMP(FoundSubstringsKeeper& solution, TextMeta& text, Substring& substring, uint32_t distance) {

	for (size_t i = 0; i < text.stringBuff.size(); i++) {

		while (substring.matchesUpToNow != 0 && (text.stringBuff[i] != substring.string[substring.matchesUpToNow])) {
			substring.matchesUpToNow = substring.table[substring.matchesUpToNow - 1];
		}

		if (text.stringBuff[i] == substring.string[substring.matchesUpToNow]) {
			substring.matchesUpToNow++;
		}

		if (substring.matchesUpToNow == (short)substring.string.size()) {					//if the whole substring was matched, try to find a pair for it
			pairSubstrings(text, solution, calculatePosition(text, substring), distance);
			substring.matchesUpToNow = substring.table[substring.matchesUpToNow - 1];
		}

		if (text.stringBuff[i] == '\n') {
			text.row_length.push_back(text.col + 1);
			text.row++;
			text.col = 0;
		}
		else {
			text.col++;
		}

	}

}


int main(int argc, char* argv[]){

	try {

		if (argc != 4) throw "Invalid number of parameters.";

		FoundSubstringsKeeper solution;
		TextMeta text;
		Substring substring;

		std::ifstream ifs = fileLoading(argv[1]);
		uint32_t distance = distanceLoading(argv[3]);

		substring.string = substringLoading(argv[2]);;
		substring.table = buildTable(substring.string);
		text.stringBuff.resize(MAX_LOADED_CHARS_NUM);

		while (ifs.read(text.stringBuff.data(), text.stringBuff.size())) {
			KMP(solution, text, substring, distance);
		}
		std::streamsize k = ifs.gcount();
		text.stringBuff.resize(k);
		KMP(solution, text, substring, distance);


		for (std::pair<uint64_t, uint64_t> pair : solution.positions) {
			std::cout << pair.first << " " << pair.second << std::endl;
		}

	}
	catch (const char* e){
		std::cout << e;
		return 1;
	}

	return 0;
}