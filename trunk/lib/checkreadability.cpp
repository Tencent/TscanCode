#include "check.h"
#include "symboldatabase.h"
#include "checkreadability.h"
#include <stack>
namespace {
	checkReadability inst;
}

/**
* check if more than 4 scope is nested
*/
void checkReadability::CheckNestedScope()
{
	if (!_settings->IsCheckIdOpened("suspicious", "tooManyNestedScopes"))
	{
		return;
	}
	enum
	{
		eNestScope = 4
	};
	const SymbolDatabase* symbolDatabase = _tokenizer->getSymbolDatabase();
	const std::vector<const Scope*>& func_list = symbolDatabase->functionScopes;

	enum { eBufSize = 11 };
	char szStr[eBufSize];
	for (std::vector<const Scope*>::const_iterator I = func_list.begin(), E = func_list.end(); I != E; ++I)
	{
		if (!*I || !(*I)->classStart || !(*I)->classEnd)
		{
			continue;
		}

		std::stack<std::pair<const Token*, bool > > s;
		std::stack<bool> sElse;
		bool bPushed = false;
		unsigned int maxScope = 0;
		const Token *tokPos = nullptr;
		for (const Token* tok = (*I)->classStart->next(), *tok_end = (*I)->classEnd; tok != tok_end; tok = tok->next())
		{
			if (tok->str() == "{")
			{
				bool bValid = false;
				bool bElse = false;
				if (tok->previous()->str() == ")")
				{
					const Scope *scope = tok->scope();
					if (scope
						&& scope->GetScopeType() == Scope::eIf
						&& scope->nestedIn
						&& scope->nestedIn->GetScopeType() == Scope::eElse
						&& scope->nestedIn->classDef->linenr() == scope->classDef->linenr()
						)
					{
						continue;
					}
					bValid = true;
				}
				else if (tok->previous()->str() == "else"
					&& tok->scope() 
					&& tok->scope()->nestedIn 
					&& tok->scope()->nestedIn->GetScopeType() != Scope::eElse)
				{
					bValid = true;
					bElse = true;
				}
				

				if (bValid)
				{
					bPushed = true;
					s.push(std::make_pair(tok, bElse));
				}
			}
			else if (tok->str() == "}")
			{
				const std::size_t nSize = s.size();
				if (nSize <= 0)
				{
					break; //bail out
				}
				if (tok->link() != s.top().first)
				{
					continue;
				}
				if (bPushed && !s.top().first->isExpandedMacro() && nSize > maxScope)
				{
					tokPos = s.top().first;
					maxScope = nSize;
				}
				s.pop();
				bPushed = false;
			}
		}
		if (maxScope >= eNestScope)
		{
			SNPRINTF(szStr, eBufSize, "%d", maxScope);
			reportError(tokPos, Severity::information, ErrorType::Suspicious, "tooManyNestedScopes"
				, "there are " + std::string(szStr) + " scopes nested in, witch may affect the readability of the function"
				, ErrorLogger::GenWebIdentity("{"));
		}
	}
}

