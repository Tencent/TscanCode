#pragma once

class TSCANCODELIB checkReadability : public Check
{
public:
	checkReadability() : Check(myName())
	{ }

	/** @brief This constructor is used when running checks. */
	checkReadability(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger)
	{ }

	static std::string myName() {
		return "CheckReadability";
	}
	std::string classInfo() const {
		return "Check if there is readability issues:\n"
			"* scopes nested too many times\n";
	}

	void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
	{
		checkReadability c(tokenizer, settings, errorLogger);
		c.CheckNestedScope();
	}

	void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) {
	}
	void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const {
	
	}
private:
	void CheckNestedScope();
};

