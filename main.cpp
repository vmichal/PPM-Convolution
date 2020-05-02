//Homework assignment for university course Computer architectures at CTU FEE
//https://cw.fel.cvut.cz/wiki/courses/b35apo/homeworks/02/start

#pragma GCC optimize("Ofast")

#include <iostream>
#include <iterator>
#include <array>
#include <algorithm>
#include <vector>
#include <cassert>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <numeric>

constexpr auto file_magic = "P6";


int print_help(std::string program_name) {
	std::cerr <<
		"Usage: " << program_name << " argument\n"
		"Argument shall be a valid path to ppm picture.\n";

	return 0;
}

bool check_magic(std::ifstream& input) {
	std::string line;
	std::getline(input, line);
	return line == file_magic;
}

struct pixel {
	std::uint_fast8_t r, g, b;
};

static_assert(sizeof(pixel) == 3);

constexpr int clamp(int current) {
	return current > 255 ? 255 : current < 0 ? 0 : current;
}

std::array<int, 6> histogram{ {0} };

void add_to_histogram(pixel const p) {
	uint8_t const greyscale = std::round(p.r * 0.2126 + p.g * 0.7152 + p.b * 0.0722);

	histogram[greyscale / 51]++;
}


int main(int argc, char** argv) {

	std::ios_base::sync_with_stdio(false);

	if (argc != 2) {
		std::cout << "Incorrect number of arguments.\n";
		return print_help(argv[0]);
	}

	std::ifstream input(argv[1]);

	if (!input.is_open() || !check_magic(input)) {
		std::cerr << "This is not a ppm file!\n";
		return print_help(argv[0]);
	}
	if (input.peek() == '#') {
		std::string comment; //Extract single comment line from input file
		std::getline(input, comment);
	}

	//Read dimensions

	std::int_fast32_t W, H, max_value;
	input >> W >> H >> max_value;
	assert(W > 0 && H > 0 && max_value == 255);
	input.ignore(1); //ignore the last newline after header
	std::int_fast32_t const width = W, height = H;

	std::int_fast32_t const size = width * height;

	std::vector<pixel> pixels(size); //Allocate memory for all pixels
	input.read(reinterpret_cast<char*>(pixels.data()), size * sizeof(pixel)); //and read them from binary file

	std::ostringstream output_header; //Header of output file
	output_header << file_magic << '\n' << width << '\n' << height << '\n' << max_value << '\n';
	std::string const header = output_header.str();

	std::ofstream output_file("output.ppm", std::ios::binary);
	output_file << header; //Write header to output file 
	output_file.write(reinterpret_cast<char*>(pixels.data()), width * sizeof(pixel)); //write the first line straight away

	for (std::int_fast32_t i = width; i > 0;) { //Compute histogram for the first and last line 
		add_to_histogram(pixels[size - i]);
		--i;
		add_to_histogram(pixels[i]);
	}

	for (std::int_fast32_t row = 0; row < height - 2; ++row) {
		pixel* const previous = pixels.data() + row * width,
			* const current = previous + width,
			* const next = current + width;

		//first pixel 
		add_to_histogram(current[0]);
		previous[0] = current[0];


		//Main body of the matrix
		std::int_fast32_t remaining = width - 2;
		for (std::int_fast32_t col = 1; remaining; ++col, --remaining) {

			uint8_t const red = clamp(5 * current[col].r - current[col - 1].r - current[col + 1].r - previous[col].r - next[col].r);
			uint8_t const green = clamp(5 * current[col].g - current[col - 1].g - current[col + 1].g - previous[col].g - next[col].g);
			uint8_t const blue = clamp(5 * current[col].b - current[col - 1].b - current[col + 1].b - previous[col].b - next[col].b);

			add_to_histogram({ red, green, blue });
			previous[col] = { red, green, blue };
		}

		//last pixel
		add_to_histogram(current[width - 1]);
		previous[width - 1] = current[width - 1];
	}

	histogram[4] += histogram[5]; //accounting for the hacky histogram computation

	//write modified inner part of picture
	output_file.write(reinterpret_cast<char*>(pixels.data()), width * (height - 2) * sizeof(pixel));

	//write unmodified last line
	output_file.write(reinterpret_cast<char*>(pixels.data() + width * (height - 1)), width * sizeof(pixel));

	{ //Print histogram to file
		std::ofstream histogram_output("output.txt");
		std::copy(histogram.begin(), histogram.begin() + 4, std::ostream_iterator<int>(histogram_output, " "));
		histogram_output << histogram[4];

	}
	return 0;

}