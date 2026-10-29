#include "xdfwriter.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

static int failures = 0;

static void check(bool condition, const std::string &message) {
	if (!condition) {
		std::cerr << "FAIL: " << message << "\n";
		++failures;
	}
}

static std::string contents_of(const std::string &filename) {
	std::ifstream file(filename, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static void writes_info_entries_into_the_file_header() {
	const std::string filename = "test_header_info.xdf";
	{ XDFWriter w(filename, {{"protocol", "{\"a\":1}"}, {"protocol-sha256", "abc123"}}); }

	const std::string header = contents_of(filename);
	std::remove(filename.c_str());

	const auto protocol = header.find("<protocol>{&quot;a&quot;:1}</protocol>");
	const auto sha = header.find("<protocol-sha256>abc123</protocol-sha256>");
	const auto end = header.find("</info>");

	check(protocol != std::string::npos, "Did not write the protocol entry!");
	check(sha != std::string::npos, "Did not write the protocol-sha256 entry!");
	check(protocol < end && sha < end, "Did not write the entries inside <info>!");
	check(header.find("<version>1.0</version>") != std::string::npos,
		"Did not keep the version!");
}

static void escapes_info_values() {
	const std::string filename = "test_header_escape.xdf";
	{ XDFWriter w(filename, {{"note", "<a & 'b'>"}}); }

	const std::string header = contents_of(filename);
	std::remove(filename.c_str());

	check(header.find("<note>&lt;a &amp; &apos;b&apos;&gt;</note>") != std::string::npos,
		"Did not escape the info value!");
}

static void writes_the_same_header_without_info() {
	const std::string filename = "test_header_plain.xdf";
	{ XDFWriter w(filename); }

	const std::string header = contents_of(filename);
	std::remove(filename.c_str());

	const auto version = header.find("<version>1.0</version>");
	const auto datetime = header.find("</datetime>");
	const auto end = header.find("</info>");

	check(version != std::string::npos && datetime != std::string::npos &&
			  end != std::string::npos,
		"Did not write the plain header!");
	check(header.substr(datetime + 11, end - datetime - 11) == "\n  ",
		"Wrote something extra into the plain header!");
}

static bool throws_for_key(const std::string &key) {
	const std::string filename = "test_header_invalid.xdf";
	bool threw = false;

	try {
		XDFWriter w(filename, {{key, "value"}});
	} catch (const std::invalid_argument &) { threw = true; }

	std::remove(filename.c_str());
	return threw;
}

static void rejects_keys_that_are_not_xml_names_or_are_reserved() {
	check(throws_for_key("has space"), "Did not reject a key with a space!");
	check(throws_for_key("1starts-with-digit"), "Did not reject a key starting with a digit!");
	check(throws_for_key(""), "Did not reject an empty key!");
	check(throws_for_key("<tag>"), "Did not reject a key with angle brackets!");
	check(throws_for_key("version"), "Did not reject the reserved version key!");
	check(throws_for_key("datetime"), "Did not reject the reserved datetime key!");
	check(!throws_for_key("protocol-sha256"), "Rejected a valid key!");
}

static void leaves_an_existing_file_alone_when_a_key_is_invalid() {
	const std::string filename = "test_header_existing.xdf";
	{ std::ofstream(filename) << "keep me"; }

	try {
		XDFWriter w(filename, {{"not valid", "value"}});
	} catch (const std::invalid_argument &) {}

	const std::string contents = contents_of(filename);
	std::remove(filename.c_str());

	check(contents == "keep me", "Truncated an existing file for an invalid key!");
}

int main() {
	writes_info_entries_into_the_file_header();
	escapes_info_values();
	writes_the_same_header_without_info();
	rejects_keys_that_are_not_xml_names_or_are_reserved();
	leaves_an_existing_file_alone_when_a_key_is_invalid();

	if (failures == 0) { std::cout << "All header tests passed.\n"; }
	return failures == 0 ? 0 : 1;
}
