/*
 * TscanCode - A tool for static C/C++ code analysis
 * Copyright (C) 2017 TscanCode team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "errorlogger.h"
#include "path.h"
#include "tscancode.h"
#include "tokenlist.h"
#include "token.h"
#include "symboldatabase.h"

#include <tinyxml2.h>

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>

#include <iostream>
#include <fstream>
using namespace std;


InternalError::InternalError(const Token *tok, const std::string &errorMsg, Type type) :
    token(tok), errorMessage(errorMsg)
{
    switch (type) {
    case SYNTAX:
        id = "syntaxError";
        break;
    case INTERNAL:
        id = "TscanCodeError";
        break;
    }
}

ErrorLogger::ErrorMessage::ErrorMessage()
    : _severity(Severity::none), _cwe(0U), _inconclusive(false)
{
}

ErrorLogger::ErrorMessage::ErrorMessage(const std::list<FileLocation> &callStack, Severity::SeverityType severity, const std::string &msg, ErrorType::ErrorTypeEnum type, const std::string& id, bool inconclusive) :
    _callStack(callStack), // locations for this error message
    _type(type),               // set the message id
	_id(id),
    _severity(severity),   // severity for this error message
    _cwe(0U),
    _inconclusive(inconclusive)
{
    // set the summary and verbose messages
    setmsg(msg);
}

ErrorLogger::ErrorMessage::ErrorMessage(const std::list<const Token*>& callstack, const TokenList* list, Severity::SeverityType severity, ErrorType::ErrorTypeEnum type, const std::string& id, const std::string& msg, bool inconclusive)
    : _type(type), _id(id), _severity(severity), _cwe(0U), _inconclusive(inconclusive)
{
    // Format callstack
    for (std::list<const Token *>::const_iterator it = callstack.begin(); it != callstack.end(); ++it) {
        // --errorlist can provide null values here
        if (!(*it))
            continue;
        _callStack.push_back(ErrorLogger::ErrorMessage::FileLocation(*it, list));
    }

    if (list && !list->getFiles().empty())
        file0 = list->getFiles()[0];

    setmsg(msg);
}

void ErrorLogger::ErrorMessage::setmsg(const std::string &msg)
{
    // If a message ends to a '\n' and contains only a one '\n'
    // it will cause the _verboseMessage to be empty which will show
    // as an empty message to the user if --verbose is used.
    // Even this doesn't cause problems with messages that have multiple
    // lines, none of the the error messages should end into it.
    assert(!(*msg.rbegin() =='\n'));

    // The summary and verbose message are separated by a newline
    // If there is no newline then both the summary and verbose messages
    // are the given message
    const std::string::size_type pos = msg.find('\n');
    if (pos == std::string::npos) {
        _shortMessage = msg;
        _verboseMessage = msg;
    } else {
        _shortMessage = msg.substr(0, pos);
        _verboseMessage = msg.substr(pos + 1);
    }
}

std::string ErrorLogger::ErrorMessage::serialize() const
{
    // Serialize this message into a simple string
    std::ostringstream oss;
    oss << _id.length() << " " << _id;
    oss << Severity::toString(_severity).length() << " " << Severity::toString(_severity);
    oss << MathLib::toString(_cwe).length() << " " << MathLib::toString(_cwe);
    if (_inconclusive) {
        const std::string inconclusive("inconclusive");
        oss << inconclusive.length() << " " << inconclusive;
    }

    const std::string saneShortMessage = fixInvalidChars(_shortMessage);
    const std::string saneVerboseMessage = fixInvalidChars(_verboseMessage);

    oss << saneShortMessage.length() << " " << saneShortMessage;
    oss << saneVerboseMessage.length() << " " << saneVerboseMessage;
    oss << _callStack.size() << " ";

    for (std::list<ErrorLogger::ErrorMessage::FileLocation>::const_iterator tok = _callStack.begin(); tok != _callStack.end(); ++tok) {
        std::ostringstream smallStream;
        smallStream << (*tok).line << ":" << (*tok).getfile();
        oss << smallStream.str().length() << " " << smallStream.str();
    }

    return oss.str();
}

bool ErrorLogger::ErrorMessage::deserialize(const std::string &data)
{
    _inconclusive = false;
    _callStack.clear();
    std::istringstream iss(data);
    std::vector<std::string> results;
    while (iss.good()) {
        unsigned int len = 0;
        if (!(iss >> len))
            return false;

        iss.get();
        std::string temp;
        for (unsigned int i = 0; i < len && iss.good(); ++i) {
            char c = static_cast<char>(iss.get());
            temp.append(1, c);
        }

        if (temp == "inconclusive") {
            _inconclusive = true;
            continue;
        }

        results.push_back(temp);
        if (results.size() == 5)
            break;
    }

    if (results.size() != 5)
        throw InternalError(0, "Internal Error: Deserialization of error message failed");

    _id = results[0];
    _severity = Severity::fromString(results[1]);
    std::istringstream scwe(results[2]);
    scwe >> _cwe;
    _shortMessage = results[3];
    _verboseMessage = results[4];

    unsigned int stackSize = 0;
    if (!(iss >> stackSize))
        return false;

    while (iss.good()) {
        unsigned int len = 0;
        if (!(iss >> len))
            return false;

        iss.get();
        std::string temp;
        for (unsigned int i = 0; i < len && iss.good(); ++i) {
            char c = static_cast<char>(iss.get());
            temp.append(1, c);
        }

        ErrorLogger::ErrorMessage::FileLocation loc;
        loc.setfile(temp.substr(temp.find(':') + 1));
        temp = temp.substr(0, temp.find(':'));
        std::istringstream fiss(temp);
        fiss >> loc.line;

        _callStack.push_back(loc);

        if (_callStack.size() >= stackSize)
            break;
    }

    return true;
}

std::string ErrorLogger::ErrorMessage::getXMLHeader(int xml_version)
{
    // xml_version 1 is the default xml format

    tinyxml2::XMLPrinter printer;

    // standard xml header
    printer.PushDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");

    // header
    printer.OpenElement("results", false);
    // version 2 header
    if (xml_version == 2) {
        printer.PushAttribute("version", xml_version);
        printer.OpenElement("tscancode", false);
        printer.PushAttribute("version", TscanCode::version());
        printer.CloseElement(false);
        printer.OpenElement("errors", false);
    }

    return std::string(printer.CStr()) + '>';
}

std::string ErrorLogger::ErrorMessage::getXMLFooter(int xml_version)
{
    return (xml_version<=1) ? "</results>" : "    </errors>\n</results>";
}

// There is no utf-8 support around but the strings should at least be safe for to tinyxml2.
// See #5300 "Invalid encoding in XML output" and  #6431 "Invalid XML created - Invalid encoding of string literal "
std::string ErrorLogger::ErrorMessage::fixInvalidChars(const std::string& raw)
{
    std::string result;
    result.reserve(raw.length());
    std::string::const_iterator from=raw.begin();
    while (from!=raw.end()) {
        if (std::isprint(static_cast<unsigned char>(*from))) {
            result.push_back(*from);
        } else {
            std::ostringstream es;
            // straight cast to (unsigned) doesn't work out.
            const unsigned uFrom = (unsigned char)*from;
#if 0
            if (uFrom<0x20)
                es << "\\XXX";
            else
#endif
                es << '\\' << std::setbase(8) << std::setw(3) << std::setfill('0') << uFrom;
            result += es.str();
        }
        ++from;
    }
    return result;
}

std::string ErrorLogger::ErrorMessage::toXML(bool verbose, int version) const
{
	// The default xml format
	if (version == 1) {
		tinyxml2::XMLPrinter printer(0, false, 1);
		printer.OpenElement("error", false);
		if (!_callStack.empty()) {
			printer.PushAttribute("file", _callStack.back().getfile().c_str());
			printer.PushAttribute("line", _callStack.back().line);
		}
		printer.PushAttribute("id", ErrorType::ToString(_type).c_str());
		printer.PushAttribute("subid", _id.c_str());
		printer.PushAttribute("severity", Settings::Instance()->GetCheckSeverity(_id.c_str()).c_str());
		printer.PushAttribute("msg", fixInvalidChars(verbose ? _verboseMessage : _shortMessage).c_str());
		printer.PushAttribute("web_identify", fixInvalidChars(_webIdentify).c_str());
		printer.PushAttribute("func_info", fixInvalidChars(_funcinfo).c_str());

		if (_inconclusive)
			printer.PushAttribute("inconclusive", "true");
		printer.CloseElement(false);
		return printer.CStr();
	}

	return "";
}

static std::string stringToXml(std::string s)
{
	// convert a string so it can be save as xml attribute data
	std::string::size_type pos = 0;
	while ((pos = s.find_first_of("<>&\"\n", pos)) != std::string::npos) {
		if (s[pos] == '<')
			s.insert(pos + 1, "&lt;");
		else if (s[pos] == '>')
			s.insert(pos + 1, "&gt;");
		else if (s[pos] == '&')
			s.insert(pos + 1, "&amp;");
		else if (s[pos] == '"')
			s.insert(pos + 1, "&quot;");
		else if (s[pos] == '\n')
			s.insert(pos + 1, "&#xa;");
		s.erase(pos, 1);
		++pos;
	}
	return s;
}

std::string ErrorLogger::ErrorMessage::toXML_codetrace(bool verbose, int version) const
{
	// The default xml format
	tinyxml2::XMLPrinter printer(0, false, 1);
	printer.OpenElement("error", false);
	if (_callStack.empty()) {
		return "";
	}
	printer.PushAttribute("file", _callStack.back().getfile().c_str());
	printer.PushAttribute("line", _callStack.back().line);
	
	printer.PushAttribute("id", ErrorType::ToString(_type).c_str());
	printer.PushAttribute("subid", _id.c_str());
	printer.PushAttribute("severity", Settings::Instance()->GetCheckSeverity(_id.c_str()).c_str());
	printer.PushAttribute("msg", fixInvalidChars(verbose ? _verboseMessage : _shortMessage).c_str());
	printer.PushAttribute("web_identify", fixInvalidChars(_webIdentify).c_str());
	printer.PushAttribute("func_info", fixInvalidChars(_funcinfo).c_str());

	std::string filename = stringToXml(_callStack.back().getfile());
	unsigned int fline = _callStack.back().line;

	fstream fFile(filename.c_str());

	std::string strTextLine = "", content = "";
	if (fFile.is_open())
	{
		int i = 1;
		int startline = 0, endline = 0;
		while (getline(fFile, strTextLine, '\n'))
		{
			if (fline > 10)
				startline = fline - 10;
			else
				startline = 1;
			endline = 10 + fline;
			char sline[20];
			sprintf(sline, "%d", i);
			if (i >= startline && i <= endline)
			{
				content += string(sline) + ": " + strTextLine + "\n";
			}
			i++;
		}
		fFile.close();

		printer.PushAttribute("content", content.c_str());

		if (_inconclusive)
			printer.PushAttribute("inconclusive", "true");
		printer.CloseElement(false);
		return printer.CStr();
	}
	
	return "";
	
}

void ErrorLogger::ErrorMessage::findAndReplace(std::string &source, const std::string &searchFor, const std::string &replaceWith)
{
    std::string::size_type index = 0;
    while ((index = source.find(searchFor, index)) != std::string::npos) {
        source.replace(index, searchFor.length(), replaceWith);
        index += replaceWith.length();
    }
}

std::string ErrorLogger::ErrorMessage::toString(bool verbose, const std::string &outputFormat) const
{
    // Save this ErrorMessage in plain text.
    // No template is given
    if (outputFormat.length() == 0) {
        std::ostringstream text;
        if (!_callStack.empty())
            text << callStackToString(_callStack) << ": ";

		text << "(" << Settings::Instance()->GetCheckSeverity(_id) << ") ";
        
        text << (verbose ? _verboseMessage : _shortMessage);
        return text.str();
    }

    // template is given. Reformat the output according to it
    else {
        std::string result = outputFormat;
        findAndReplace(result, "\\b", "\b");
        findAndReplace(result, "\\n", "\n");
        findAndReplace(result, "\\r", "\r");
        findAndReplace(result, "\\t", "\t");

        findAndReplace(result, "{id}", _id);
        findAndReplace(result, "{severity}", Settings::Instance()->GetCheckSeverity(_id));
        findAndReplace(result, "{message}", verbose ? _verboseMessage : _shortMessage);
        findAndReplace(result, "{callstack}", _callStack.empty() ? "" : callStackToString(_callStack));
        if (!_callStack.empty()) {
            std::ostringstream oss;
            oss << _callStack.back().line;
            findAndReplace(result, "{line}", oss.str());
            findAndReplace(result, "{file}", _callStack.back().getfile());
        } else {
            findAndReplace(result, "{file}", "");
            findAndReplace(result, "{line}", "");
        }

        return result;
    }
}

void ErrorLogger::reportUnmatchedSuppressions(const std::list<Suppressions::SuppressionEntry> &unmatched)
{
    // Report unmatched suppressions
    for (std::list<Suppressions::SuppressionEntry>::const_iterator i = unmatched.begin(); i != unmatched.end(); ++i) {
        // don't report "unmatchedSuppression" as unmatched
        if (i->id == "unmatchedSuppression")
            continue;

        // check if this unmatched suppression is suppressed
        bool suppressed = false;
        for (std::list<Suppressions::SuppressionEntry>::const_iterator i2 = unmatched.begin(); i2 != unmatched.end(); ++i2) {
            if (i2->id == "unmatchedSuppression") {
                if ((i2->file == "*" || i2->file == i->file) &&
                    (i2->line == 0 || i2->line == i->line)) {
                    suppressed = true;
                    break;
                }
            }
        }

        if (suppressed)
            continue;

        std::list<ErrorLogger::ErrorMessage::FileLocation> callStack;
        callStack.push_back(ErrorLogger::ErrorMessage::FileLocation(i->file, i->line));
        reportErr(ErrorLogger::ErrorMessage(callStack, Severity::information, "Unmatched suppression: " + i->id, ErrorType::None, "unmatchedSuppression", false));
    }
}

std::string ErrorLogger::callStackToString(const std::list<ErrorLogger::ErrorMessage::FileLocation> &callStack)
{
    std::ostringstream ostr;
    for (std::list<ErrorLogger::ErrorMessage::FileLocation>::const_iterator tok = callStack.begin(); tok != callStack.end(); ++tok) {
        ostr << (tok == callStack.begin() ? "" : " -> ") << tok->stringify();
    }
    return ostr.str();
}


ErrorLogger::ErrorMessage::FileLocation::FileLocation(const Token* tok, const TokenList* list)
    : line(tok->linenr()), _file(list->file(tok))
{
}

std::string ErrorLogger::ErrorMessage::FileLocation::getfile(bool convert) const
{
    if (convert)
        return Path::toNativeSeparators(_file);
    return _file;
}

void ErrorLogger::ErrorMessage::FileLocation::setfile(const std::string &file)
{
    _file = file;
    _file = Path::fromNativeSeparators(_file);
    _file = Path::simplifyPath(_file);
}

std::string ErrorLogger::ErrorMessage::FileLocation::stringify() const
{
    std::ostringstream oss;
    oss << '[' << Path::toNativeSeparators(_file);
    if (line != 0)
        oss << ':' << line;
    oss << ']';
    return oss.str();
}



const std::string ErrorLogger::GenWebIdentity(const std::string &idenity, const std::string &retfrom)
{
	std::ostringstream ostr;
	if (retfrom.empty())
	{
		ostr << "{\"identify\":\"" << idenity << "\"}";
	}
	else
	{
		ostr << "{\"identify\":\"" << idenity << "\",\"returnfrom\":\"" << retfrom << "\"}";
	}
	return ostr.str();
}

const std::string ErrorLogger::GenWebIdentity(const std::string &idenity)
{
	return GenWebIdentity(idenity, std::string());
}



void ErrorLogger::GetScopeFuncInfo(const Scope* s, std::string &_lastFuncName)
{
	if (!s)
	{
		_lastFuncName.clear();
		return;
	}



	if (s->type == Scope::eFunction)
	{
		if (!s->classStart || !s->classEnd || !s->classDef)
		{
			_lastFuncName.clear();
			return;
		}
		Token *funcStart = const_cast<Token*>(s->classDef);
		while (funcStart->previous() && s->classDef->linenr() == funcStart->linenr() && !Token::Match(funcStart->previous(), "{|}|;|public:|protected:|private:"))
		{
			funcStart = funcStart->previous();
		}
		if (!funcStart)
		{
			_lastFuncName.clear();
			return;
		}
		std::string strFuncInfo;
		if (!s->nestedIn || s->nestedIn->className.empty())
		{
			for (const Token *tok = funcStart; tok && tok != s->classStart; tok = tok->next())
			{
				strFuncInfo += tok->str();
				if (tok->str() != "::" && !Token::Match(tok->next(), "::"))
				{
					strFuncInfo += " ";
				}
			}
		}
		else
		{
			if (Token::Match(s->classDef->previous(), "::"))
			{
				for (const Token *tok = funcStart; tok && tok != s->classStart; tok = tok->next())
				{
					strFuncInfo += tok->str();
					if (tok->str() != "::" && !Token::Match(tok->next(), "::"))
					{
						strFuncInfo += " ";
					}
				}
			}
			else
			{
				for (const Token *tok = funcStart; tok && tok != s->classDef; tok = tok->next())
				{
					strFuncInfo += tok->str();
					if (tok->str() != "::" && !Token::Match(tok->next(), "::"))
					{
						strFuncInfo += " ";
					}
				}
				strFuncInfo += s->nestedIn->className;
				strFuncInfo += "::";
				for (const Token *tok = s->classDef; tok && tok != s->classStart; tok = tok->next())
				{
					strFuncInfo += tok->str();
					strFuncInfo += " ";
				}
			}
		}
		if (strFuncInfo.empty())
		{
			_lastFuncName.clear();
			return;
		}
		_lastFuncName = strFuncInfo.substr(0, strFuncInfo.length() - 1);
		return;
	}
	else if ( s->type < Scope::eFunction)
	{
		if (s->type == Scope::eGlobal)
			_lastFuncName = "global";
		else
			_lastFuncName = s->className;

		while (s->nestedIn && !s->nestedIn->className.empty())
		{
			_lastFuncName = s->nestedIn->className + "::" + _lastFuncName;
			s = s->nestedIn;
		}
		return;
	}
	
	_lastFuncName.clear();
	return;
}
