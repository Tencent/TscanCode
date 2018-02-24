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


//---------------------------------------------------------------------------
#include "checkuninitvar.h"
#include "astutils.h"
#include "mathlib.h"
#include "checknullpointer.h"   // CheckNullPointer::parseFunctionCall
#include "symboldatabase.h"
#include "globaltokenizer.h"
#include "utilities.h"
#include <algorithm>
#include <map>
#include <cassert>
#include <stack>
//---------------------------------------------------------------------------
#define POSSIBLE_PARAM_USE 2
#define PARAM_USE 1
// Register this check class (by creating a static instance of it)
namespace {
    CheckUninitVar instance;
}

//---------------------------------------------------------------------------
#include "checkclass.h"
void CheckUninitVar::CheckCtor()
{
	bCheckCtor = true;
	bGoInner = true;
	tokCaller = nullptr;
	const std::vector<const Scope*>& cands = _tokenizer->getSymbolDatabase()->classAndStructScopes;
	for (std::size_t i = 0, classCount = cands.size(); i < classCount; ++i) {
		const Scope * scope = cands[i];

		// There are no constructors.
		if (scope->numConstructors == 0) {
			continue;
		}

		// #3196 => bailout if there are nested unions
		// TODO: handle union variables better
		{
			bool bailout = false;
			for (std::list<Scope *>::const_iterator it = scope->nestedList.begin(), end = scope->nestedList.end(); it != end; ++it) {
				const Scope * const nestedScope = *it;
				if (nestedScope->type == Scope::eUnion) {
					bailout = true;
					break;
				}
			}
			if (bailout)
				continue;
		}

		std::vector<CheckClass::Usage> usage(scope->varlist.size());

		for (std::list<Function>::const_iterator func = scope->functionList.begin(), endFunc = scope->functionList.end(); func != endFunc; ++func) {
			if (!func->hasBody() || !(func->isConstructor() ||
				func->type == Function::eOperatorEqual))
				continue;

			// Mark all variables not used
			for (std::size_t ii = 0, nSize = usage.size(); ii < nSize; ++ii) {
				usage[ii].assign = false;
				usage[ii].init = false;
			}

			std::list<const Function *> callstack;
			CheckClass::initializeVarList(*func, callstack, scope, usage, true);

			//if construct func is null,not report error
			if (Token::Match(func->tokenDef->next(), "( ) { }") || Token::Match(func->token->next(), "( ) { }"))//Constructor declaration and Constructor definition 
			{
				continue;
			}
			//end

			// Check if any variables are uninitialized
			std::list<Variable>::const_iterator var;
			unsigned int count = 0;
			for (var = scope->varlist.begin(); var != scope->varlist.end(); ++var, ++count) {

				if (var->hasDefault() || usage[count].assign || usage[count].init || var->isStatic())
					continue;

				if (var->isConst() && func->isOperator()) // We can't set const members in assignment operator
					continue;

				// Check if this is a class constructor
				if (!var->isPointer() && !var->isPointerArray() && var->isClass() && func->type == Function::eConstructor) {
					// Unknown type so assume it is initialized
					if (!var->type())
						continue;

					// Known type that doesn't need initialization or
					// known type that has member variables of an unknown type
					else if (var->type()->needInitialization != Type::True)
						continue;
				}

				// Check if type can't be copied
				if (!var->isPointer() && !var->isPointerArray() && var->typeScope()) {
					if (func->type == Function::eMoveConstructor) {
						if (CheckClass::canNotMove(var->typeScope()))
							continue;
					}
					else {
						if (CheckClass::canNotCopy(var->typeScope()))
							continue;
					}
				}

				// Don't warn about unknown types in copy constructors since we
				// don't know if they can be copied or not..
				if (!var->isPointer() &&
					!(var->type() && var->type()->needInitialization != Type::True) &&
					(func->type == Function::eCopyConstructor || func->type == Function::eOperatorEqual)) {
					if (!var->typeStartToken()->isStandardType()) {
							continue;
					}
				}

				if (func->type != Function::eOperatorEqual && func->functionScope) {
					Alloc alloc = var->isArray() ? ARRAY : NO_ALLOC;
					bCtorIf = false;
					//function has body
					checkScopeForVariable(func->functionScope->classStart, *var, nullptr, nullptr, &alloc, emptyString, false);
				}
			}
		}
	}
	bCheckCtor = false;
	tokCaller = nullptr;
	bCtorIf = false;
}

void CheckUninitVar::check()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
    std::list<Scope>::const_iterator scope;

    // check every executable scope
    for (scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) {
        if (scope->isExecutable()) {
            checkScope(&*scope);
        }
    }

	CheckCtor();
}

void CheckUninitVar::checkScope(const Scope* scope)
{
    for (std::list<Variable>::const_iterator i = scope->varlist.begin(); i != scope->varlist.end(); ++i) {
        if ((_tokenizer->isCPP() && i->type() && !i->isPointer() && i->type()->needInitialization != Type::True) ||
            i->isStatic() || i->isExtern() || i->isReference())
            continue;

        // don't warn for try/catch exception variable
        if (i->isThrow())
            continue;

        if (i->nameToken()->strAt(1) == "(" || i->nameToken()->strAt(1) == "{"  || i->nameToken()->strAt(1) == ":")
            continue;

        if (Token::Match(i->nameToken(), "%name% =")) { // Variable is initialized, but Rhs might be not
            checkRhs(i->nameToken(), *i, NO_ALLOC, 0U, "", false);
            continue;
        }
        if (Token::Match(i->nameToken(), "%name% ) (") && Token::simpleMatch(i->nameToken()->linkAt(2), ") =")) { // Function pointer is initialized, but Rhs might be not
            checkRhs(i->nameToken()->linkAt(2)->next(), *i, NO_ALLOC, 0U, "", false);
            continue;
        }

        if (i->isArray() || i->isPointerToArray()) {
            const Token *tok = i->nameToken()->next();
            if (i->isPointerToArray())
                tok = tok->next();
            while (Token::simpleMatch(tok->link(), "] ["))
                tok = tok->link()->next();
            if (Token::Match(tok->link(), "] =|{"))
                continue;
        }

        bool stdtype = _tokenizer->isC();
        const Token* tok = i->typeStartToken();
        for (; tok && tok != i->nameToken() && tok->str() != "<"; tok = tok->next()) {
            if (tok->isStandardType())
                stdtype = true;
        }
        if (i->isArray() && !stdtype)
            continue;

        while (tok && tok->str() != ";")
            tok = tok->next();
        if (!tok)
            continue;

        if (i->isArray()) {
            Alloc alloc = ARRAY;
            checkScopeForVariable(tok, *i, nullptr, nullptr, &alloc, "", false);
            continue;
        }
        if (stdtype || i->isPointer()) {
            Alloc alloc = NO_ALLOC;
            checkScopeForVariable(tok, *i, nullptr, nullptr, &alloc, "", false);
        }
		
		checkStruct(tok, *i);
		
    }

    if (scope->function) {
        for (unsigned int i = 0; i < scope->function->argCount(); i++) {
            const Variable *arg = scope->function->getArgumentVar(i);
            if (arg && arg->declarationId() && Token::Match(arg->typeStartToken(), "struct| %type% * %name% [,)]")) {
                // Treat the pointer as initialized until it is assigned by malloc
                for (const Token *tok = scope->classStart; tok != scope->classEnd; tok = tok->next()) {
                    if (Token::Match(tok, "[;{}] %varid% = %name% (", arg->declarationId()) &&
                        _settings->library.returnuninitdata.count(tok->strAt(3)) == 1U) {
                        if (arg->typeStartToken()->str() == "struct")
                            checkStruct(tok, *arg);
                        else if (arg->typeStartToken()->isStandardType()) {
                            Alloc alloc = NO_ALLOC;
                            checkScopeForVariable(tok->next(), *arg, nullptr, nullptr, &alloc, "", false);
                        }
                    }
                }
            }
        }
    }
}

/**
* return true if type is null
*/
static bool IsVarSysType(const Variable &var)
{
	if (!var.typeEndToken() || !var.typeStartToken())
	{
		return true;
	}

	const Library &lib = Settings::Instance()->library;
	for (const Token *t = var.typeStartToken(), *end = var.typeEndToken()->next(); t != end; t = t->next())
	{
		if (t->isName() && (
			Token::Match(t, "bool|char|short|int|long|float|double|wchar_t|unsigned|signed")
			|| nullptr != lib.podtype(t->str()) 
			|| lib.containers.count(t->str()) != 0)
			)
		{
			return true;
		}
	}

	return false;
}


void CheckUninitVar::checkStruct(const Token *tok, const Variable &structvar)
{
	if (structvar.type())
	{
		const Token *typeToken = structvar.typeStartToken();
		if (typeToken->str() == "struct")
			typeToken = typeToken->next();

		const SymbolDatabase * symbolDatabase = _tokenizer->getSymbolDatabase();
		for (std::size_t j = 0U; j < symbolDatabase->classAndStructScopes.size(); ++j) {
			const Scope *scope2 = symbolDatabase->classAndStructScopes[j];
			if (scope2->className == typeToken->str() && (scope2->numConstructors == 0U
				|| (scope2->definedType && scope2->definedType->needInitialization != Type::True)))
			{
				for (std::list<Variable>::const_iterator it = scope2->varlist.begin(); it != scope2->varlist.end(); ++it) {
					const Variable &var = *it;
					if (var.hasDefault()
						|| var.isArray()
						|| var.isStatic()
						|| (var.nameToken() && var.nameToken()->scope() && Token::simpleMatch(var.nameToken()->scope()->classDef, "union"))
						|| (!_tokenizer->isC() && var.isClass() && (!var.type() || var.type()->needInitialization != Type::True)))
					{
						continue;
					}

					Alloc alloc = NO_ALLOC;
					const Token *tok2 = tok;
					if (tok->str() == "}")
						tok2 = tok2->next();
					checkScopeForVariable(tok2, structvar, nullptr, nullptr, &alloc, var.name(), var.isPointer());
				}
				ReportUninitStructError(structvar.name(), tok, false, _uninitMem.begin(), _uninitMem.end());
				_uninitMem.clear();
				ReportUninitStructError(structvar.name(), tok, true,  _uninitPossibleMem.begin(), _uninitPossibleMem.end());
				_uninitPossibleMem.clear();
			}
		}
	}
	else if (!IsVarSysType(structvar))//cannot find type
	{
		const gt::CScope *gs = CGlobalTokenizer::Instance()->GetGlobalScope(&structvar);
		if (!gs || !gs->NeedInit())
		{
			return;
		}
		for (std::map<std::string, gt::CField*>::const_iterator iter = gs->GetFieldMap().begin(), end = gs->GetFieldMap().end();
			iter != end; ++iter)
		{
			if (iter->second->hasDefault() 
				|| !iter->second->needInit() 
				|| iter->second->isArray()
				|| iter->second->isStatic())
			{
				continue;
			}
			Alloc alloc = NO_ALLOC;
			const Token *tok2 = tok;
			if (tok->str() == "}")
				tok2 = tok2->next();
			checkScopeForVariable(tok2, structvar, nullptr, nullptr, &alloc, iter->first, iter->second->isPointer());
		}
		ReportUninitStructError(structvar.name(), tok, false, _uninitMem.begin(), _uninitMem.end());
		_uninitMem.clear();
		ReportUninitStructError(structvar.name(), tok, true, _uninitPossibleMem.begin(), _uninitPossibleMem.end());
		_uninitPossibleMem.clear();
	}
}


static void conditionAlwaysTrueOrFalse(const Token *tok, const std::map<unsigned int, int> &variableValue, bool *alwaysTrue, bool *alwaysFalse)
{
    assert(Token::simpleMatch(tok, "if ("));

    const Token *vartok = tok->tokAt(2);
    const bool NOT(vartok->str() == "!");
    if (NOT)
        vartok = vartok->next();

    while (Token::Match(vartok, "%name% . %name%"))
        vartok = vartok->tokAt(2);

    std::map<unsigned int, int>::const_iterator it = variableValue.find(vartok->varId());
    if (it == variableValue.end())
        return;

    // always true
    if (Token::Match(vartok, "%name% %oror%|)")) {
        if (NOT)
            *alwaysTrue = bool(it->second == 0);
        else
            *alwaysTrue = bool(it->second != 0);
    } else if (Token::Match(vartok, "%name% == %num% %or%|)")) {
        *alwaysTrue = bool(it->second == MathLib::toLongNumber(vartok->strAt(2)));
    } else if (Token::Match(vartok, "%name% != %num% %or%|)")) {
        *alwaysTrue = bool(it->second != MathLib::toLongNumber(vartok->strAt(2)));
    }

    // always false
    if (Token::Match(vartok, "%name% &&|)")) {
        if (NOT)
            *alwaysFalse = bool(it->second != 0);
        else
            *alwaysFalse = bool(it->second == 0);
    } else if (Token::Match(vartok, "%name% == %num% &&|)")) {
        *alwaysFalse = bool(it->second != MathLib::toLongNumber(vartok->strAt(2)));
    } else if (Token::Match(vartok, "%name% != %num% &&|)")) {
        *alwaysFalse = bool(it->second == MathLib::toLongNumber(vartok->strAt(2)));
    }
}

static bool isVariableUsed(const Token *tok, const Variable& var)
{
    if (!tok)
        return false;
    if (tok->str() == "&" && !tok->astOperand2())
        return false;
    if (tok->isConstOp())
        return isVariableUsed(tok->astOperand1(),var) || isVariableUsed(tok->astOperand2(),var);
    if (tok->varId() != var.declarationId())
        return false;
    if (!var.isArray())
        return true;

    const Token *parent = tok->astParent();
    while (Token::Match(parent, "[?:]"))
        parent = parent->astParent();
    // no dereference, then array is not "used"
    if (!Token::Match(parent, "*|["))
        return false;
    const Token *parent2 = parent->astParent();
    // TODO: handle function calls. There is a TODO assertion in TestUninitVar::uninitvar_arrays
    return !parent2 || parent2->isConstOp() || (parent2->str() == "=" && parent2->astOperand2() == parent);
}



bool CheckUninitVar::checkIfForWhileHead(const Token *startparentheses, const Variable& var, bool suppressErrors, bool isuninit, Alloc alloc, const std::string &membervar, bool bPointer)
{
    const Token * const endpar = startparentheses->link();
    if (Token::Match(startparentheses, "( ! %name% %oror%") && startparentheses->tokAt(2)->getValue(0))
        suppressErrors = true;
    for (const Token *tok = startparentheses->next(); tok && tok != endpar; tok = tok->next()) {
        if (tok->varId() == var.declarationId()) {
            if (Token::Match(tok, "%name% . %name%")) {
                if (tok->strAt(2) == membervar) {
                    if (isMemberVariableAssignment(tok, membervar, bPointer))
                        return true;
					int use;
                    if (!suppressErrors && (use = isMemberVariableUsage(tok, var.isPointer(), alloc, membervar, bPointer)) > 0)
                        uninitStructMemberError(tok, membervar, use == POSSIBLE_PARAM_USE);
                }
				else if (tok->strAt(3) == "(") // func, consider as inited
				{
					return true;
				}
                continue;
            }
			int use;
            if ((use = isVariableUsage(tok, var.isPointer(), alloc)) > 0) {
                if (suppressErrors)
                    continue;
                uninitvarError(tok, tok->str(), alloc, use == POSSIBLE_PARAM_USE);
            }
            return true;
        }
        if (Token::Match(tok, "sizeof|decltype|offsetof ("))
            tok = tok->next()->link();
        if ((!isuninit || !membervar.empty()) && tok->str() == "&&")
            suppressErrors = true;
    }
    return false;
}


enum LoopSate
{
	eFalse,
	eTrue,
	eBreak,
	eContinue
};

bool CheckUninitVar::checkLoopBody(const Token *tok, const Variable& var, Alloc &alloc, const std::string &membervar, 
	bool bPointer, bool * const possibleInit, const bool suppressErrors, bool * const noreturn)
{
	const Token *usetok = nullptr;
	int use = 0;
	assert(tok->str() == "{");

	for (const Token * const end = tok->link(); tok != end; tok = tok->next()) {
		if (tok->varId() == var.declarationId()) {
			if (!membervar.empty()) {
				if (isMemberVariableAssignment(tok, membervar, bPointer)) {
					bool assign = true;
					bool rhs = false;
					for (const Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
						if (tok2->str() == "=")
							rhs = true;
						if (tok2->str() == ";")
							break;
						if (rhs && tok2->varId() == var.declarationId() 
							&& (use = isMemberVariableUsage(tok2, var.isPointer(), alloc, membervar, bPointer)) > 0) {
							assign = false;
							break;
						}
					}
					if (assign)
						return true;
				}

				if (Token::Match(tok, "%name% ="))
					return true;

				if ((use = isMemberVariableUsage(tok, var.isPointer(), alloc, membervar, bPointer)) > 0)
					usetok = tok;
				else if (Token::Match(tok->previous(), "[(,] %name% [,)]"))
					return true;
			}
			else {
				if ((use = isVariableUsage(tok, var.isPointer(), alloc)) > 0)
					usetok = tok;
				else if (tok->strAt(1) == "=") {
					// Is var used in rhs?
					bool rhs = false;
					std::stack<const Token *> tokens;
					tokens.push(tok->next()->astOperand2());
					while (!tokens.empty()) {
						const Token *t = tokens.top();
						tokens.pop();
						if (!t)
							continue;
						if (t->varId() == var.declarationId()) {
							// var is used in rhs
							rhs = true;
							break;
						}
						if (Token::simpleMatch(t->previous(), "sizeof ("))
							continue;
						tokens.push(t->astOperand1());
						tokens.push(t->astOperand2());
					}
					if (!rhs)
						return true;
				}
				else {
					return true;
				}
			}
		}

		if (Token::Match(tok, "sizeof|typeof ("))
			tok = tok->next()->link();

		if (Token::Match(tok, "asm ( %str% ) ;"))
			return true;
	}

	if (!suppressErrors && usetok) {
		if (membervar.empty())
			uninitvarError(usetok, usetok->str(), alloc, use == POSSIBLE_PARAM_USE);
		else
			uninitStructMemberError(usetok, membervar, use == POSSIBLE_PARAM_USE);
		return true;
	}

	return false;
}

void CheckUninitVar::checkRhs(const Token *tok, const Variable &var, Alloc alloc, unsigned int number_of_if, const std::string &membervar, bool bPointer)
{
    bool rhs = false;
    unsigned int indent = 0;
	int use;
    while (nullptr != (tok = tok->next())) {
        if (tok->str() == "=")
            rhs = true;
        else if (rhs && tok->varId() == var.declarationId()) {
            if (membervar.empty() && ((use = isVariableUsage(tok, var.isPointer(), alloc)) > 0))
                uninitvarError(tok, tok->str(), alloc, use == POSSIBLE_PARAM_USE);
            else if (!membervar.empty() && (use = isMemberVariableUsage(tok, var.isPointer(), alloc, membervar, bPointer)) > 0)
                uninitStructMemberError(tok, membervar, use == POSSIBLE_PARAM_USE);

        } else if (tok->str() == ";" || (indent==0 && tok->str() == ","))
            break;
        else if (tok->str() == "(")
            ++indent;
        else if (tok->str() == ")") {
            if (indent == 0)
                break;
            --indent;
        } else if (tok->str() == "?" && tok->astOperand2()) {
            const bool used1 = isVariableUsed(tok->astOperand2()->astOperand1(), var);
            const bool used0 = isVariableUsed(tok->astOperand2()->astOperand2(), var);
            const bool err = (number_of_if == 0) ? (used1 || used0) : (used1 && used0);
            if (err)
                uninitvarError(tok, var.nameToken()->str(), alloc, false);
            break;
        } else if (Token::simpleMatch(tok, "sizeof ("))
            tok = tok->next()->link();
    }
}

int CheckUninitVar::isVariableUsage(const Token *vartok, bool pointer, Alloc alloc, const std::string &member) const
{
    if (alloc == NO_ALLOC && ((Token::Match(vartok->previous(), "return|delete") && vartok->strAt(1) != "=") || (vartok->strAt(-1) == "]" && vartok->linkAt(-1)->strAt(-1) == "delete")))
        return true;

    // Passing variable to typeof/__alignof__
    if (Token::Match(vartok->tokAt(-3), "typeof|__alignof__ ( * %name%"))
        return false;

    // Accessing Rvalue member using "." or "->"
    if (vartok->strAt(1) == "." && vartok->strAt(-1) != "&") {
        // Is struct member passed to function?
        if (!pointer && Token::Match(vartok->previous(), "[,(] %name% . %name%")) {
            // TODO: there are FN currently:
            // - should only return false if struct member is (or might be) array.
            // - should only return false if function argument is (or might be) non-const pointer or reference
            const Token *tok2 = vartok->next();
            do {
                tok2 = tok2->tokAt(2);
            } while (Token::Match(tok2, ". %name%"));
            if (Token::Match(tok2, "[,)]"))
                return false;
        } 
		else if (pointer && alloc != CTOR_CALL && Token::Match(vartok, "%name% . %name% (")) {
            return true;
        }
		else if (!pointer && Token::Match(vartok, "%name% . %name%"))
		{
			const Token *tokMem = vartok->next()->next();
			if (Token::simpleMatch(tokMem->next(), "(") || 
				(!tokMem->variable() || !tokMem->variable()->type() || tokMem->variable()->type()->needInitialization != Type::True))
			{
				return false;
			}
		}
		
        bool assignment = false;
        const Token* parent = vartok->astParent();
        while (parent) {
            if (parent->str() == "=") {
                assignment = true;
                break;
            }
            if (alloc != NO_ALLOC && parent->str() == "(") {
                if (_settings->library.functionpure.find(parent->strAt(-1)) == _settings->library.functionpure.end()) {
                    assignment = true;
                    break;
                }
            }
            parent = parent->astParent();
        }
        if (!assignment)
            return true;
    }


    if (Token::Match(vartok->previous(), "[(,] %name% [,)]") || Token::Match(vartok->tokAt(-2), "[(,] & %name% [,)]")) {
        const int use = isFunctionParUsage(vartok, pointer, alloc, member);
        if (use >= 0)
            return use;
    }

    if (Token::Match(vartok->previous(), "++|--|%cop%")) {
        if (_tokenizer->isCPP() && alloc == ARRAY && Token::Match(vartok->tokAt(-4), "& %var% =|( *"))
            return false;
#ifdef USE_STRICT_OPER_CHECK
        if (_tokenizer->isCPP() && Token::Match(vartok->previous(), ">>|<<")) {
            const Token* tok2 = vartok->previous();
            if (Token::simpleMatch(tok2->astOperand1(), ">>"))
                return false; // Looks like stream operator, initializes the variable
            if (Token::simpleMatch(tok2, "<<")) {
                // Looks like stream operator, but could also initialize the variable. Check lhs.
                do {
                    tok2 = tok2->astOperand1();
                } while (Token::simpleMatch(tok2, "<<"));
                if (tok2 && tok2->strAt(-1) == "::")
                    tok2 = tok2->previous();
                if (tok2 && (Token::simpleMatch(tok2->previous(), "std ::") || (tok2->variable() && tok2->variable()->isStlType()) || tok2->isStandardType()))
                    return true;
            }
            const Variable *var = vartok->tokAt(-2)->variable();
            return (var && var->typeStartToken()->isStandardType());
        }
#else
		if (_tokenizer->isCPP())
		{
			//warning: both operator is treated as initializer
			if (Token::Match(vartok->astParent(), ">>|<<"))
			{
				return false;
			}
		}
#endif
        // is there something like: ; "*((&var ..expr.. ="  => the variable is assigned
        if (vartok->previous()->str() == "&" && !vartok->previous()->astOperand2())
            return false;

        // bailout to avoid fp for 'int x = 2 + x();' where 'x()' is a unseen preprocessor macro (seen in linux)
        if (!pointer && vartok->next() && vartok->next()->str() == "(")
            return false;

        if (vartok->previous()->str() != "&" || !Token::Match(vartok->tokAt(-2), "[(,=?:]")) {
            if (alloc != NO_ALLOC && vartok->previous()->str() == "*") {
                const Token *parent = vartok->previous()->astParent();
                if (parent && parent->str() == "=" && parent->astOperand1() == vartok->previous())
                    return false;
				//f(int **p){return *p;}; int *p; f(&p);
				if (pointer 
					&& vartok->variable() 
					&& vartok->variable()->nameToken() 
					&& Token::simpleMatch(vartok->variable()->nameToken()->tokAt(-2), "* *")
					&& alloc != NO_ALLOC
					&& Token::simpleMatch(vartok->astParent(), "*")
					&& !Token::simpleMatch(vartok->astParent()->astParent(), "*")
					)
				{
					return false;
				}
				//memset(&*req,xxx,xxx)
				if (Token::simpleMatch(vartok->astParent(), "*") 
					&& Token::simpleMatch(vartok->astParent()->astParent(), "&")
					&& !Token::simpleMatch(vartok->astParent()->astParent()->astParent(), "*")
					&&
						(!vartok->variable()
							|| !Token::Match(vartok->variable()->typeEndToken(), "iterator|const_iterator|reverse_iterator|move_iterator")
						)
					)
				{
					return false;
				}
                return true;
            }
            return alloc == NO_ALLOC;
        }
    }

    if (alloc == NO_ALLOC && Token::Match(vartok->previous(), "= %name% ;|%cop%")) {
        // taking reference?
		// a &= 0
		if (vartok->astParent())
		{
			if (vartok->astParent()->str() == "=" && vartok->variable()->type() && vartok->variable()->type()->classScope)
			{
				return POSSIBLE_PARAM_USE;
			}

			if (vartok->astParent()->str() == "&"
				&& Token::simpleMatch(vartok->astParent()->astOperand2(), "0"))
			{
				return false;
			}
		}
		
        const Token *prev = vartok->tokAt(-2);
        while (Token::Match(prev, "%name%|*"))
            prev = prev->previous();
		if (!Token::simpleMatch(prev, "&"))
		{
			return true;
		}
    }

    bool unknown = false;
    if (pointer && alloc == NO_ALLOC && CheckNullPointer::isPointerDeRef(vartok, unknown)) {
        // function parameter?
        bool functionParameter = false;
        if (Token::Match(vartok->tokAt(-2), "%name% (") || vartok->previous()->str() == ",")
            functionParameter = true;

        // if this is not a function parameter report this dereference as variable usage
        if (!functionParameter)
            return true;
    } else if (alloc != NO_ALLOC && Token::Match(vartok, "%var% [")) {
        const Token *parent = vartok->next()->astParent();
		if (parent == nullptr 
			&& Token::simpleMatch(vartok->next(), "[") 
			&& Token::simpleMatch(vartok->next()->link(), "] ="))
		{
			return false;
		}
        while (Token::Match(parent, "[|."))
            parent = parent->astParent();
        if (Token::simpleMatch(parent, "&") && !parent->astOperand2())
            return false;
        if (parent && Token::Match(parent->previous(), "if|while|switch ("))
            return true;
        if (Token::Match(parent, "[=,(]"))
            return false;
        return true;
    }

    if (_tokenizer->isCPP() && Token::Match(vartok->next(), "<<|>>")) {
        // Is this calculation done in rhs?
        const Token *tok = vartok;
        while (tok && Token::Match(tok, "%name%|.|::"))
            tok = tok->previous();
        if (Token::Match(tok, "[;{}]"))
            return false;

        // Is variable a known POD type then this is a variable usage,
        // otherwise we assume it's not.
        const Variable *var = vartok->variable();
        return (var && var->typeStartToken()->isStandardType());
    }

    if (alloc == NO_ALLOC && vartok->next() && vartok->next()->isOp() && !vartok->next()->isAssignmentOp())
        return true;

    if (vartok->strAt(1) == "]")
        return true;

    return false;
}


/**
start goes to (
*/
static inline unsigned int ComputeArgPos(const Token *&start)
{
	unsigned int argumentNumber = 0;
	while (start && !Token::Match(start, "[;{}(]")) {
		if (start->str() == ")")
			start = start->link();
		else if (start->str() == ",")
			++argumentNumber;
		start = start->previous();
	}
	return argumentNumber;
}

static inline bool IsPointerOffsetAsArg(const Token *tokBody)
{
	return Token::Match(tokBody->astParent(), "+|-|++|--")
		&& tokBody->astParent()->astOperand1()
		&& Token::Match(tokBody->astParent()->astOperand1()->previous(), "[(,]")
		&& Token::Match(tokBody->astParent()->astOperand2(), "%any% [,)]");
}

/***
 * Is function parameter "used" so a "usage of uninitialized variable" can
 * be written? If parameter is passed "by value" then it is "used". If it
 * is passed "by reference" then it is not necessarily "used".
 * @return  -1 => unknown   0 => not used   1 => used, 2-> possible use
 */
int CheckUninitVar::isFunctionParUsage(const Token *vartok, bool pointer, Alloc alloc, const std::string &member) const
{
    if (!Token::Match(vartok->previous(), "[(,]") && !Token::Match(vartok->tokAt(-2), "[(,] &"))
        return -1;

    // locate start parentheses in function call..
    
    const Token *start = vartok;
	unsigned int argumentNumber = ComputeArgPos(start);

    // is this a function call?
    if (start && Token::Match(start->previous(), "%name% (")) {
        const bool address(vartok->previous()->str() == "&");
        const bool array(vartok->variable() && vartok->variable()->isArray());
        // check how function handle uninitialized data arguments..
        const Function *func = start->previous()->function();
        if (func) {
            const Variable *arg = func->getArgumentVar(argumentNumber);
            if (arg) {
				//go into func to check. If this arg is used in the func 
				if (func->functionScope != NULL && arg->nameToken() && arg->nameToken()->varId() > 0) {

					Alloc alc = alloc;
					if (address)
					{
						alc = CTOR_CALL;
					}
					else if (vartok->variable()->isArray())
					{
						alc = ARRAY;
					}

					bool bpt = arg->nameToken()->variable()->isPointer();
					bool bDoublePtr = bpt && Token::simpleMatch(arg->nameToken()->tokAt(-2), "* *");

					if (!member.empty())
					{
						for (const Token* tokBody = func->functionScope->classStart; tokBody&&tokBody != func->functionScope->classEnd; tokBody = tokBody->next())
						{
							if (tokBody->varId() != arg->declarationId())
							{
								continue;
							}
							if (tokBody->previous()->str() == "&" || Token::simpleMatch(tokBody->next(), "="))
							{
								return 0;
							}
							if (Token::Match(tokBody->previous(), "[(,] %name% [,)]") || Token::Match(tokBody->tokAt(-2), "[(,] & %name% [,)]")
								|| (bpt && IsPointerOffsetAsArg(tokBody))
								)
							{
								const Token *ftok = tokBody;
								unsigned int argPos = ComputeArgPos(ftok);
								if ((!bpt || alc == NO_ALLOC || alloc == ARRAY)
									&& ftok && ftok->previous()
									&& _settings->library.isuninitargbad(ftok->previous(), argPos + 1))
								{
									return 1;
								}
								return 0;
							}
							if (Token::Match(tokBody, "%name% . %name%") && tokBody->strAt(2) == member)
							{
								if (tokBody->strAt(3) == "=")
								{
									return 0;
								}
								if (tokBody->astTop()->str() == "=" )
								{
									if (tokBody->astUnderTopLeft())
									{
										return 0;
									}
									//int &a = xxx.m1;
									else if (Token::Match(tokBody->astTop()->tokAt(-4), "!!const %type% & %name%"))
									{
										return 0;
									}
								}
								if (Token::Match(tokBody->previous(), "[(,]") && Token::Match(tokBody->tokAt(3), "[,)]"))
								{
									return 0;
								}
								return 1;
							}
						}
						return 0;
					}
					else
					{
						for (const Token* tokBody = func->functionScope->classStart; tokBody&&tokBody != func->functionScope->classEnd; tokBody = tokBody->next())
						{
							if (tokBody->varId() != arg->declarationId())
							{
								continue;
							}
							if (Token::Match(tokBody->previous(), "[(,] %name% [,)]") || Token::Match(tokBody->tokAt(-2), "[(,] & %name% [,)]")
								|| (bpt && IsPointerOffsetAsArg(tokBody))
								)
							{
								const Token *ftok = tokBody;
								unsigned int argPos = ComputeArgPos(ftok);
								if ((!bpt || alc == NO_ALLOC || alloc == ARRAY)
									&& ftok && ftok->previous()
									&& _settings->library.isuninitargbad(ftok->previous(), argPos + 1))
								{
									return 1;
								}
								return 0;
							}
							else if (Token::simpleMatch(tokBody->astParent(), "&"))
							{
								return 0;
							}
							else
							{
								bool bUnderSizeOf = false;
								const Token *tokAst = tokBody->astParent();
								const Token *tokChild = tokBody;
								while (tokAst && tokAst->str() != "=")
								{
									tokChild = tokAst;
									if (tokChild->str() == "(" && Token::Match(tokChild->astOperand1(), SIZE_OF_PATTEN))
									{
										bUnderSizeOf = true;
										break;
									}
									tokAst = tokAst->astParent();
								}
								if (bUnderSizeOf)
								{
									continue;
								}
								if (tokAst && tokAst->str() == "=")
								{
									if (tokAst->astOperand2() == tokChild) //char *p = param;*p = xxx;
									{
										if ((!Token::Match(tokBody->astParent(), "*|[")
											&& !(Token::Match(tokBody->astParent(), "+|-")
												&& Token::simpleMatch(tokBody->astParent()->astOperand1(), "*")
												)
											&& (!tokAst->astOperand1()
												|| !tokAst->astOperand1()->variable()
												|| tokAst->astOperand1()->variable()->isPointer()
												)
											)
											//void foo(void **p) { void *q = *p; }
											|| (bDoublePtr && (address || alloc == CTOR_CALL)
												&& Token::simpleMatch(tokBody->astParent(), "*")
												&& !Token::simpleMatch(tokBody->astParent()->astOperand1(), "*")
												)
											)
										{
											return 0;
										}
									}
									else
									{
										return 0;
									}
								}
								if (isVariableUsage(tokBody, bpt, alc))
								{
									return 1;
								}
							}
						}
						return 0;
					}
				}
				const Token *argStart = arg->typeStartToken();
				if (!address && !array && (!pointer || alloc == NO_ALLOC) && Token::Match(argStart, "struct| %type% %name%| [,)]"))
					return POSSIBLE_PARAM_USE;
				if (pointer && !address && alloc == NO_ALLOC && Token::Match(argStart, "struct| %type% * %name% [,)]"))
					return POSSIBLE_PARAM_USE;
				while (argStart->previous() && argStart->previous()->isName())
					argStart = argStart->previous();
				if (Token::Match(argStart, "const %type% & %name% [,)]"))
					return POSSIBLE_PARAM_USE;
				if ((pointer || address) && alloc == NO_ALLOC && Token::Match(argStart, "const struct| %type% * %name% [,)]"))
					return POSSIBLE_PARAM_USE;
				if ((pointer || address) && Token::Match(argStart, "const %type% %name% [") && Token::Match(argStart->linkAt(3), "] [,)]"))
					return POSSIBLE_PARAM_USE;
            }

        } else if (Token::Match(start->previous(), "if|while|for|switch")) {
            // control-flow statement reading the variable "by value"
            return alloc == NO_ALLOC;
        } else {
			//check if argument must be initialized
            const bool isnullbad = _settings->library.isnullargbad(start->previous(), argumentNumber + 1);
            if (pointer && !address && isnullbad && alloc == NO_ALLOC)
                return true;
            const bool isuninitbad = _settings->library.isuninitargbad(start->previous(), argumentNumber + 1);
            if (alloc != NO_ALLOC)
                return isnullbad && isuninitbad;

			// can not find func info in library, so return -1
			if (!isnullbad && !isuninitbad)
			{
				return -1;
			}
            return isuninitbad && (!address || isnullbad);
        }
    }

    // unknown
    return -1;
}

bool CheckUninitVar::isMemberVariableAssignment(const Token *tok, const std::string &membervar, bool bPointer) const
{
    if (Token::Match(tok, "%name% . %name%") && tok->strAt(2) == membervar) 
	{
        if (Token::Match(tok->tokAt(3), "[=.[]"))
            return true;
        else if (Token::Match(tok->tokAt(-2), "[(,=] &"))
            return true;
        else if (Token::Match(tok->tokAt(-2), "%name% >>") && Token::Match(tok->tokAt(3), ";|>>")) // #6680
            return true;
        else if ((tok->previous() && tok->previous()->isConstOp()) || Token::Match(tok->previous(), "[|="))
            ; // member variable usage
        else if (tok->tokAt(3)->isConstOp())
            ; // member variable usage
        else if (Token::Match(tok->previous(), "[(,] %name% . %name% [,)]") &&
                 1 == isFunctionParUsage(tok, bPointer, NO_ALLOC, emptyString)) {
            return false;
		}
		else
		{
			return true;
		}
    } 
	else if (Token::Match(tok->previous(), "[(,] %name% [,)]") && -1 == isFunctionParUsage(tok, bPointer, NO_ALLOC, membervar))
	{
		return true;
	}
	else if (tok->strAt(1) == "=")
        return true;
    else if (tok->strAt(-1) == "&") {
        if (Token::Match(tok->tokAt(-2), "[(,] & %name%")) {
            // locate start parentheses in function call..
            unsigned int argumentNumber = 0;
            const Token *ftok = tok;
            while (ftok && !Token::Match(ftok, "[;{}(]")) {
                if (ftok->str() == ")")
                    ftok = ftok->link();
                else if (ftok->str() == ",")
                    ++argumentNumber;
                ftok = ftok->previous();
            }

            // is this a function call?
            ftok = ftok ? ftok->previous() : NULL;
            if (Token::Match(ftok, "%name% (")) {
                // check how function handle uninitialized data arguments..
                const Function *function = ftok->function();
                const Variable *arg      = function ? function->getArgumentVar(argumentNumber) : NULL;
                const Token *argStart    = arg ? arg->typeStartToken() : NULL;
				if (argStart)
				{
					while (argStart && argStart->previous() && argStart->previous()->isName())
						argStart = argStart->previous();
					if (Token::Match(argStart, "const struct| %type% * const| %name% [,)]"))
						return false;
				}
				else
				{
					//todo: not init == input ?
					if (_settings->library.isuninitargbad(ftok, argumentNumber + 1))
					{
						return false;
					}
				}
            }

            else if (ftok && Token::simpleMatch(ftok->previous(), "= * ("))
                return false;
        }
        return true;
    }
	// tmp.Clear();
	else if (Token::Match(tok, "%name% . %name% ("))
	{
		if (!membervar.empty())
		{
			CGlobalTokenizer* inst = CGlobalTokenizer::Instance();
			const gt::CFunction *gf = inst->FindFunctionData(tok->next()->next());
			if (gf)
			{
				return inst->IsVarInitInFunc(gf, tok->variable(), membervar) || gf->GetFuncData().AssignThis();
			}
		}
		return true;
	}
	//warning: both operator is treated as initializing
	else if (Token::simpleMatch(tok->astParent(), ">>|<<"))
	{
		return true;
	}
    return false;
}

int CheckUninitVar::isMemberVariableUsage(const Token *tok, bool isPointer, Alloc alloc, const std::string &membervar, bool bPointer) const
{
	int use = 0;
    if (Token::Match(tok->previous(), "[(,] %name% . %name% [,)]") &&
        tok->strAt(2) == membervar) {
        use = isFunctionParUsage(tok, isPointer, alloc, emptyString);
        if (use == 1)
            return true;
    }

    if (isMemberVariableAssignment(tok, membervar, bPointer))
        return false;

    if (Token::Match(tok, "%name% . %name%") && tok->strAt(2) == membervar && !(tok->tokAt(-2)->variable() && tok->tokAt(-2)->variable()->isReference()))
        return true;
    else if (!isPointer && Token::Match(tok->previous(), "[(,] %name% [,)]") && (use = isVariableUsage(tok, isPointer, alloc, membervar)) > 0)
        return use;

    else if (!isPointer && Token::Match(tok->previous(), "= %name% ;"))
        return POSSIBLE_PARAM_USE;

    // = *(&var);
    else if (!isPointer &&
             Token::simpleMatch(tok->astParent(),"&") &&
             Token::simpleMatch(tok->astParent()->astParent(),"*") &&
             Token::Match(tok->astParent()->astParent()->astParent(), "= * (| &") &&
             tok->astParent()->astParent()->astParent()->astOperand2() == tok->astParent()->astParent())
        return true;

    else if (_settings->experimental &&
             !isPointer &&
             Token::Match(tok->tokAt(-2), "[(,] & %name% [,)]") &&
             isVariableUsage(tok, isPointer, alloc))
        return true;

    return false;
}

void CheckUninitVar::uninitstringError(const Token *tok, const std::string &varname, bool strncpy_)
{
	if (tok->isExpandedMacro())
	{
		return;
	}
	reportError(tok, Severity::error, ErrorType::Uninit, "uninitstring",
		"Dangerous usage of '" + varname + "'" + (strncpy_ ? " (strncpy doesn't always null-terminate it)." : " (not null-terminated)."), ErrorLogger::GenWebIdentity(varname));
		
}

void CheckUninitVar::uninitdataError(const Token *tok, const std::string &varname, bool bPossible)
{
	if (tok->isExpandedMacro())
	{
		return;
	}
	if (tokCaller != nullptr)
	{
		enum { eBufLen = 11 };
		char szBuf[eBufLen];
		SNPRINTF(szBuf, eBufLen, "%u", tok->linenr());
		reportError(tokCaller, Severity::error, ErrorType::Uninit, ErrorType(tok, bPossible),
			"Memory is allocated but not initialized: " + varname + ". See line " + szBuf + "for more information",
			ErrorLogger::GenWebIdentity(varname));
	}
	else
	{
		reportError(tok, Severity::error, ErrorType::Uninit, ErrorType(tok, bPossible),
			"Memory is allocated but not initialized: " + varname,
			ErrorLogger::GenWebIdentity(varname));
	}
}

const char *CheckUninitVar::ErrorType(const Token * tok, bool bPossible) const
{
	if (bPossible || bCtorIf)
	{
		if (bCheckCtor)
		{
			return "possibleUninitMemberInCtor";
		}
		else if (tok && tok->variable()->isPointer())
		{
			return "possibleUninitPtr";
		}
		else
		{
			return "possibleUninitvar";
		}
	}

	if (bCheckCtor)
	{
		return "uninitMemberInCtor";
	}
	else if (tok && tok->variable() && tok->variable()->isPointer())
	{
		return "uninitPtr";
	}
	else
	{
		return "uninitvar";
	}
}

void CheckUninitVar::uninitvarError(const Token *tok, const std::string &varname, bool bPossible)
{
	if (tok->isExpandedMacro())
	{
		return;
	}

	if (tokCaller != nullptr)
	{
		enum { eBufLen = 11 };
		char szBuf[eBufLen];
		SNPRINTF(szBuf, eBufLen, "%u", tok->linenr());
		reportError(tokCaller, Severity::error, ErrorType::Uninit, ErrorType(tok, bPossible), "Uninitialized variable: "
			+ varname +". See line " + szBuf + " for more information"
			, ErrorLogger::GenWebIdentity(varname));
	}
	else
	{
		reportError(tok, Severity::error, ErrorType::Uninit, ErrorType(tok, bPossible), "Uninitialized variable: "
			+ varname
			, ErrorLogger::GenWebIdentity(varname));
	}
}

void CheckUninitVar::uninitStructMemberError(const Token *tok, const std::string &membername, bool bPossible)
{
	if (!bPossible)
	{
		_uninitMem[tok->linenr()].push_back(std::make_pair(membername, tok));
	}
	else
	{
		_uninitPossibleMem[tok->linenr()].push_back(std::make_pair(membername, tok));
	}
}



void CheckUninitVar::ReportUninitStructError(const std::string& str, const Token* tok, bool bPossible, CI_UMERR iterErr, CI_UMERR endErr)
{
	std::string errorVar;
	enum { eBufLen = 4 * sizeof(unsigned int) + 1 };
	char szBuf[eBufLen];
	for (; iterErr != endErr; ++iterErr)
	{
		for (std::list<std::pair<std::string, const Token *> >::const_iterator iterMem = iterErr->second.begin(), endMem = iterErr->second.end();
			iterMem != endMem; ++iterMem)
		{
			errorVar += iterMem->first;
			errorVar += ",";
		}
		if (!errorVar.empty())
		{
			errorVar = errorVar.substr(0, errorVar.size() - 1);
			if (tokCaller != nullptr)
			{
				if (!tokCaller->isExpandedMacro())
				{
					sprintf(szBuf, "%u", iterErr->second.begin()->second->linenr());
					reportError(tokCaller,
						Severity::error, ErrorType::Uninit,
						(bPossible ? "PossibleUninitStruct" : "UninitStruct"),
						std::string((bPossible ? "possible" : "")) + "uninitialized struct[" + str + "] member: " + errorVar
						+ ". See line " + szBuf + " for more information"
						, ErrorLogger::GenWebIdentity(str + (iterErr->second.size() > 1 ? "" : "." + iterErr->second.begin()->first)));
				}
				
			}
			else if (!iterErr->second.begin()->second->isExpandedMacro())
			{
				reportError(iterErr->second.begin()->second,
					Severity::error, ErrorType::Uninit,
					(bPossible ?  "PossibleUninitStruct" : "UninitStruct"),
					std::string((bPossible ? "possible " : "")) + "uninitialized struct[" + str + "] member: " + errorVar
					, ErrorLogger::GenWebIdentity(str + (iterErr->second.size() > 1 ? "" : "." + iterErr->second.begin()->first)));
			}
			errorVar.clear();
		}
	}
}

void CheckUninitVar::deadPointer()
{
    const SymbolDatabase *symbolDatabase = _tokenizer->getSymbolDatabase();
    std::list<Scope>::const_iterator scope;

    // check every executable scope
    for (scope = symbolDatabase->scopeList.begin(); scope != symbolDatabase->scopeList.end(); ++scope) {
        if (!scope->isExecutable())
            continue;
        // Dead pointers..
        for (const Token* tok = scope->classStart; tok != scope->classEnd; tok = tok->next()) {
            if (tok->variable() &&
                tok->variable()->isPointer() &&
                isVariableUsage(tok, true, NO_ALLOC)) {
                const Token *alias = tok->getValueTokenDeadPointer();
                if (alias) {
                    deadPointerError(tok,alias);
                }
            }
        }
    }
}

void CheckUninitVar::deadPointerError(const Token *pointer, const Token *alias)
{
    const std::string strpointer(pointer ? pointer->str() : std::string("pointer"));
    const std::string stralias(alias ? alias->expressionString() : std::string("&x"));

    reportError(pointer,
                Severity::error, ErrorType::Logic,
                "deadpointer",
				"Dead pointer usage. Pointer '" + strpointer + "' is dead if it has been assigned '" + stralias + "' at line " + MathLib::toString(alias ? alias->linenr() : 0U) + ".", ErrorLogger::GenWebIdentity(strpointer));
}


const Token *CheckUninitVar::FindGotoLabel(const std::string &label, const Token *tokStart) const
{
	if (!tokStart)
	{
		return nullptr;
	}
	for (const Token *t = tokStart->next(); t; t = t->next())
	{
		//reach function end
		if (t->tokType() == Token::eBracket && t->str() == "}" && (!t->scope() || t->scope()->type == Scope::eFunction))
		{
			return nullptr;
		}
		else if (t->tokType() == Token::eName && t->str() == label && t->next() && t->next()->str() == ":")
		{
			return t->next()->next();
		}
	}
	return nullptr;
}

const Token *CheckUninitVar::FindNextCase(const Token *tok) const
{
	int countScope = 0;
REWIND_CASE:
	while (tok && tok->str() != "case" && tok->str() != "default" && tok->next())
	{
		tok = tok->next();
		//skip inner switch
		if (tok->str() == "switch")
		{
			if (Token::simpleMatch(tok->next(), "(") && Token::simpleMatch(tok->next()->link(), ") {"))
			{
				tok = tok->next()->link()->next()->link();				
			}
		}
		else if (tok->str() == "{")
		{
			++countScope;
		}
		else if (tok->str() == "}")
		{
			if (countScope == 0)
			{
				return nullptr;
			}
			--countScope;
		}
	}
	while (tok && tok->str() != ":")
	{
		tok = tok->next();
	}
	if (tok && Token::Match(tok->next(), "case|default"))
	{
		tok = tok->next();
		goto REWIND_CASE;
	}
	return tok;
}



bool CheckUninitVar::checkScopeForVariable(const Token *tok, const Variable& var, bool * const possibleInit,
	bool * const noreturn, Alloc* const alloc, const std::string &membervar, bool bPointer)
{
	const bool suppressErrors(possibleInit && *possibleInit);
	if (possibleInit)
		*possibleInit = false;

	unsigned int number_of_if = 0;

	if (var.declarationId() == 0U)
		return true;

	// variable values
	std::map<unsigned int, int> variableValue;
	int ret = 0;
	for (; tok; tok = tok->next()) {
		ret = checkScopeTokenForVariable(tok, number_of_if, var, possibleInit, noreturn, alloc, membervar, variableValue, bPointer, suppressErrors);
		if (ret == eTrue)
		{
			return true;
		}
		if (ret == eBreak)
		{
			break;
		}
		if (ret == eFalse)
		{
			return false;
		}
	}

	return false;
}

bool CheckUninitVar::HandleSpecialCase(const Token *tok)
{
	//case 1
	//if (UPERR_CASE_NAME(XXX))
	// Macro UPERR_CASE_NAME cannot be expanded, and this macro assign value to var
	const Token *tokFunc = nullptr;
	if (Token::Match(tok->next()->next(), "%name% ("))
	{
		tokFunc = tok->next()->next();
	}
	else if (Token::Match(tok->next()->next(), "! %name% ("))
	{
		tokFunc = tok->tokAt(3);
	}
	if (tokFunc && !tokFunc->function() && Utility::IsNameLikeMacro(tokFunc->str()))
	{
		return true;
	}
	return false;
}

int CheckUninitVar::checkScopeTokenForVariable(const Token *&tok, unsigned int &number_of_if,
	const Variable& var, bool * const possibleInit, bool * const noreturn, Alloc* const alloc, const std::string &membervar
	, std::map<unsigned int, int> &variableValue, bool bPointer, bool suppressErrors)
{
	static const unsigned int NOT_ZERO = (1 << 30); // special variable value
	const bool printDebug = _settings->debugwarnings;
	// End of scope..
	if (tok->str() == "}") {
		if (number_of_if && possibleInit)
			*possibleInit = true;
		return eBreak;
	}

	// Unconditional inner scope or try..
	if (tok->str() == "{" && Token::Match(tok->previous(), ";|{|}|try")) {
		int ret = 0;
		for (const Token *tok2 = tok->next(), *link = tok->link(); tok2 != link && tok2; tok2 = tok2->next()) {
			ret = checkScopeTokenForVariable(tok2, number_of_if, var, possibleInit, noreturn, alloc, membervar, variableValue, bPointer, suppressErrors);
			if (ret == eTrue)
			{
				return true;
			}
			if (ret == eBreak)
			{
				return eBreak;
			}
		}
		tok = tok->link();
		return eContinue;
	}

	// assignment with nonzero constant..
	if (Token::Match(tok->previous(), "[;{}] %var% = - %name% ;"))
		variableValue[tok->varId()] = NOT_ZERO;

	// Inner scope..
	else if (Token::simpleMatch(tok, "if (")) {
		if (HandleSpecialCase(tok))
		{
			return eTrue;
		}

		bool alwaysTrue = false;
		bool alwaysFalse = false;

		conditionAlwaysTrueOrFalse(tok, variableValue, &alwaysTrue, &alwaysFalse);

		// initialization / usage in condition..
		if (!alwaysTrue && checkIfForWhileHead(tok->next(), var, suppressErrors, bool(number_of_if == 0), *alloc, membervar, bPointer))
			return eTrue;

		// checking if a not-zero variable is zero => bail out
		unsigned int condVarId = 0, condVarValue = 0;
		const Token *condVarTok = nullptr;
		if (Token::simpleMatch(tok, "if (") &&
			astIsVariableComparison(tok->next()->astOperand2(), "!=", "0", &condVarTok)) {
			std::map<unsigned int, int>::const_iterator it = variableValue.find(condVarTok->varId());
			if (it != variableValue.end() && it->second == NOT_ZERO)
				return true;   // this scope is not fully analysed => return true
			else {
				condVarId = condVarTok->varId();
				condVarValue = NOT_ZERO;
			}
		}

		// goto the {
		tok = tok->next()->link()->next();

		if (!tok)
			return eBreak;
		if (tok->str() == "{") {
			bool possibleInitIf(number_of_if > 0 || suppressErrors);
			bool noreturnIf = false;
			if (bCheckCtor)
			{
				bCtorIf = true;
			}
			const bool initif = !alwaysFalse && checkScopeForVariable(tok->next(), var, &possibleInitIf, &noreturnIf, alloc, membervar, bPointer);

			// bail out for such code:
			//    if (a) x=0;    // conditional initialization
			//    if (b) return; // 
			//    x++;           // it's possible x is always initialized
			if (!alwaysTrue && noreturnIf && number_of_if > 0) {
				if (printDebug) {
					std::string condition;
					for (const Token *tok2 = tok->linkAt(-1); tok2 != tok; tok2 = tok2->next()) {
						condition += tok2->str();
						if (tok2->isName() && tok2->next()->isName())
							condition += ' ';
					}
					reportError(tok, Severity::debug, ErrorType::None, "debug",
						"bailout uninitialized variable checking for '" + var.name() 
						+ "'. can't determine if this condition can be false when previous condition is false: " + condition,
						ErrorLogger::GenWebIdentity(tok->str()));
				}
				return eTrue;
			}

			if (alwaysTrue && noreturnIf)
				return eTrue;

			std::map<unsigned int, int> varValueIf;
			if (!alwaysFalse && !initif && !noreturnIf) {
				for (const Token *tok2 = tok; tok2 && tok2 != tok->link(); tok2 = tok2->next()) {
					if (Token::Match(tok2, "[;{}.] %name% = - %name% ;"))
						varValueIf[tok2->next()->varId()] = NOT_ZERO;
					else if (Token::Match(tok2, "[;{}.] %name% = %num% ;"))
						varValueIf[tok2->next()->varId()] = (int)MathLib::toLongNumber(tok2->strAt(3));
				}
			}

			if (initif && condVarId > 0U)
				variableValue[condVarId] = condVarValue ^ NOT_ZERO;

			// goto the }
			tok = tok->link();

			if (!Token::simpleMatch(tok, "} else {")) {
				if (initif || possibleInitIf) {
					++number_of_if;
					if (number_of_if >= 2)
						return eTrue;
				}
			}
			else {
				// goto the {
				tok = tok->tokAt(2);

				bool possibleInitElse(number_of_if > 0 || suppressErrors);
				bool noreturnElse = false;
				const bool initelse = !alwaysTrue && checkScopeForVariable(tok->next(), var, &possibleInitElse, &noreturnElse, alloc, membervar, bPointer);

				std::map<unsigned int, int> varValueElse;
				if (!alwaysTrue && !initelse && !noreturnElse) {
					for (const Token *tok2 = tok; tok2 && tok2 != tok->link(); tok2 = tok2->next()) {
						if (Token::Match(tok2, "[;{}.] %var% = - %name% ;"))
							varValueElse[tok2->next()->varId()] = NOT_ZERO;
						else if (Token::Match(tok2, "[;{}.] %var% = %num% ;"))
							varValueElse[tok2->next()->varId()] = (int)MathLib::toLongNumber(tok2->strAt(3));
					}
				}

				if (initelse && condVarId > 0U && !noreturnIf && !noreturnElse)
					variableValue[condVarId] = condVarValue;

				// goto the }
				tok = tok->link();

				if ((alwaysFalse || initif || noreturnIf) &&
					(alwaysTrue || initelse || noreturnElse))
					return eTrue;

				if (initif || initelse || possibleInitElse)
				{
					++number_of_if;
				}
				if (!initif && !noreturnIf)
					variableValue.insert(varValueIf.begin(), varValueIf.end());
				if (!initelse && !noreturnElse)
					variableValue.insert(varValueElse.begin(), varValueElse.end());
			}
		}
	}

	// = { .. }
	else if (Token::simpleMatch(tok, "= {")) {
		// end token
		const Token *end = tok->next()->link();

		// If address of variable is taken in the block then bail out
		if (Token::findmatch(tok->tokAt(2), "& %varid%", end, var.declarationId()))
			return eTrue;

		// Skip block
		tok = end;
		return eContinue;
	}

	// skip sizeof / offsetof
	if (Token::Match(tok, "sizeof|typeof|offsetof|decltype ("))
		tok = tok->next()->link();

	// for/while..
	else if (Token::Match(tok, "for|while (") || Token::simpleMatch(tok, "do {")) {
		const bool forwhile = Token::Match(tok, "for|while (");

		// is variable initialized in for-head (don't report errors yet)?
		if (forwhile && checkIfForWhileHead(tok->next(), var, true, false, *alloc, membervar, bPointer))
			return eTrue;

		// goto the {
		const Token *tok2 = forwhile ? tok->next()->link()->next() : tok->next();

		if (tok2 && tok2->str() == "{") {
			bool init = checkLoopBody(tok2, var, *alloc, membervar, bPointer, possibleInit, (number_of_if > 0) || suppressErrors, noreturn);


			// variable is initialized in the loop..
			if (init)
				return eTrue;

			// is variable used in for-head?
			bool initcond = false;
			if (!suppressErrors) {
				const Token *startCond = forwhile ? tok->next() : tok->next()->link()->tokAt(2);
				initcond = checkIfForWhileHead(startCond, var, false, bool(number_of_if == 0), *alloc, membervar, bPointer);
			}

			// goto "}"
			tok = tok2->link();

			// do-while => goto ")"
			if (!forwhile) {
				// Assert that the tokens are '} while ('
				if (!Token::simpleMatch(tok, "} while (")) {
					if (printDebug)
						reportError(tok, Severity::debug, ErrorType::None, "", "assertion failed '} while ('", ErrorLogger::GenWebIdentity(tok->str()));
					return eBreak;
				}

				// Goto ')'
				tok = tok->linkAt(2);

				if (!tok)
					// bailout : invalid code / bad tokenizer
					return eBreak;

				if (initcond)
					// variable is initialized in while-condition
					return eTrue;
			}
		}
	}
	else if (Token::simpleMatch(tok, "switch ("))
	{
		++number_of_if;
		if (number_of_if >= 2)
		{
			return eTrue;
		}
		const Token *tokScopeStart = tok->next()->link();
		if (Token::simpleMatch(tokScopeStart, ") {"))
		{
			tokScopeStart = tokScopeStart->next();
			const Token *tokScopeEnd = tokScopeStart->link();
			if (!tokScopeEnd)
			{
				return eTrue;
			}

			const CGlobalTokenizer *inst = CGlobalTokenizer::Instance();
			for (const Token *tok2 = tokScopeStart->next(); tok2 && tok2 != tokScopeEnd; tok2 = tok2->next())
			{
				if (tok2->str() == "case")
				{
					while (tok2 && tok2->str() != ":")
					{
						tok2 = tok2->next();
					}
					if (!tok2)
					{
						return true;
					}
				}
				else if (tok2->str() == "break")
				{
					tok = tokScopeEnd;
					return eContinue;
				}
				else if (inst->CheckIfReturn(tok2) 
					|| eTrue == checkScopeTokenForVariable(tok2, number_of_if, var, nullptr, nullptr, alloc, membervar, variableValue, bPointer, suppressErrors))
				{
					//goto to next case
					tok2 = FindNextCase(tok2);
					if (!tok2)
					{
						tok = tokScopeEnd;
						return eTrue;
					}
				}
			}
		}
		else
		{
			return eTrue;
		}
	}
	// Unknown or unhandled inner scope
	else if (Token::simpleMatch(tok, ") {") || (Token::Match(tok, "%name% {") && tok->str() != "try")) {
		if (tok->str() == "struct" || tok->str() == "union") {
			tok = tok->linkAt(1);
			return eContinue;
		}
		return eTrue;
	}

	// bailout if there is ({
	if (Token::simpleMatch(tok, "( {")) {
		return eTrue;
	}

	// bailout if there is assembler code or setjmp
	if (Token::Match(tok, "asm|setjmp (")) {
		return eTrue;
	}

	// bailout if there is a goto label
	if (Token::Match(tok, "[;{}] %name% :")) {
		return eTrue;
	}
	
	if (tok->str() == "?") {
		if (!tok->astOperand2())
			return eTrue;
		const int used1 = isVariableUsed(tok->astOperand2()->astOperand1(), var);
		const int used0 = isVariableUsed(tok->astOperand2()->astOperand2(), var);
		const bool err = (number_of_if == 0) ? (used1 || used0) : (used1 && used0);
		if (err)
			uninitvarError(tok, var.nameToken()->str(), *alloc, false);

		// Todo: skip expression if there is no error
		return eTrue;
	}

	//Token::Match(tok, "return|break|continue|throw|goto")
	if (CGlobalTokenizer::Instance()->CheckIfReturn(tok)) {
		if (tok->str() == "goto" && tok->next() && tok->next()->isName())
		{
			const std::string &labelName = tok->next()->str();
			const Token *tLabel = FindGotoLabel(labelName, tok->next()->next());
			if (tLabel != nullptr)
			{
				int ret;
				for (const Token *tok2 = tLabel->next(); tok2; tok2 = tok2->next()) {
					ret = checkScopeTokenForVariable(tok2, number_of_if, var, possibleInit, noreturn, alloc, membervar, variableValue, bPointer, suppressErrors);
					if (ret == eTrue)
					{
						return true;
					}
					if (ret == eBreak)
					{
						break;
					}
					if (ret == eFalse)
					{
						return false;
					}
				}
			}
		}
		if (noreturn)
			*noreturn = true;

		tok = tok->next();
		int use;
		while (tok && tok->str() != ";") {
			// variable is seen..
			if (tok->varId() == var.declarationId()) {
				if (!membervar.empty()) {
					if (Token::Match(tok, "%name% . %name% ;|%cop%") && tok->strAt(2) == membervar)
						uninitStructMemberError(tok, membervar, false);
					else
						return eTrue;
				}

				// Use variable
				else if (!suppressErrors && ((use = isVariableUsage(tok, var.isPointer(), *alloc)) > 0))
					uninitvarError(tok, tok->str(), *alloc, use == POSSIBLE_PARAM_USE);
				else
					// assume that variable is assigned
					return eTrue;
			}

			else if (Token::Match(tok, "sizeof|typeof|offsetof|decltype ("))
				tok = tok->linkAt(1);

			else if (tok->str() == "?") {
				if (!tok->astOperand2())
					return eTrue;
				const bool used1 = isVariableUsed(tok->astOperand2()->astOperand1(), var);
				const bool used0 = isVariableUsed(tok->astOperand2()->astOperand2(), var);
				const bool err = (number_of_if == 0) ? (used1 || used0) : (used1 && used0);
				if (err)
					uninitvarError(tok, var.nameToken()->str(), *alloc, false);
				return eTrue;
			}

			tok = tok->next();
		}

		return noreturn == nullptr ? eTrue : eFalse;
	}

	if (bCheckCtor && Token::Match(tok->previous(), "!!. %name% ("))
	{
		if (tok->function()
			&& tok->function()->IsMemberMethod()
			&& tok->function()->functionScope
			)
		{
			if (bGoInner)
			{
				tokCaller = tok;
				if (CheckMemUseInFun(var, tok->function(), *alloc))
				{
					tokCaller = nullptr;
					return true;
				}
				tokCaller = nullptr;
			}
			else
			{
				const Token *tokAst = nullptr;
				const Token *tokChild = nullptr;
				//check if Inited in this function
				for (const Token *loopBody = tok->function()->functionScope->classStart, *endLoop = loopBody->link();
				loopBody != endLoop; loopBody = loopBody->next())
				{
					if (loopBody->varId() != var.declarationId())
					{
						continue;
					}
					if (loopBody->previous()->str() == "&")
					{
						return true;
					}
					if (Token::Match(loopBody->previous(), "[(,] &| %name% [,)]"))
					{
						return true;
					}
					tokAst = loopBody->astParent();
					tokChild = loopBody;
					while (tokAst && tokAst->str() != "=")
					{
						tokChild = tokAst;
						tokAst = tokAst->astParent();
					}
					if (tokAst)
					{
						if (tokChild == tokAst->astOperand1() || var.isPointer())
						{
							return eTrue;
						}
					}
				}
			}
		}
		else if (Token::Match(tok, ZERO_MEM_PATTERN) && tok->strAt(2) == "this")
		{
			return eTrue;
		}
	}


	// variable is seen..
	if (tok->varId() == var.declarationId()) {
		// calling function that returns uninit data through pointer..
		if (var.isPointer() &&
			Token::Match(tok->next(), "= %name% (") &&
			Token::simpleMatch(tok->linkAt(3), ") ;") &&
			_settings->library.returnuninitdata.count(tok->strAt(2)) > 0U) {
			*alloc = NO_CTOR_CALL;
			return eContinue;
		}
		if (var.isPointer() && (var.typeStartToken()->isStandardType() || (var.type() && var.type()->needInitialization == Type::True)) && Token::simpleMatch(tok->next(), "= new")) {
			*alloc = CTOR_CALL;
			if (var.typeScope() && var.typeScope()->numConstructors > 0)
				return true;
			return eContinue;
		}


		if (!membervar.empty()) {
			if (isMemberVariableAssignment(tok, membervar, bPointer)) {
				checkRhs(tok, var, *alloc, number_of_if, membervar, bPointer);
				return eTrue;
			}

			int use;
			if (!suppressErrors && (use = isMemberVariableUsage(tok, var.isPointer(), *alloc, membervar, bPointer)) > 0)
				uninitStructMemberError(tok, membervar, use == POSSIBLE_PARAM_USE);

			else if (Token::Match(tok->previous(), "[(,] %name% [,)]"))
				return eTrue;

		}
		else {
			// Use variable
			if (Token::Match(var.typeStartToken(), "std| ::| ostringstream|stringstream|iostringstream|ostream|istream|fstream|ifstream|ofstream"))
				return eTrue;
			int use;
			if (!suppressErrors && ((use = isVariableUsage(tok, var.isPointer(), *alloc)) > 0))
				uninitvarError(tok, tok->str(), *alloc, use == POSSIBLE_PARAM_USE);
			else {
				if (tok->strAt(1) == "=")
					checkRhs(tok, var, *alloc, number_of_if, "", false);

				// assume that variable is assigned
				return eTrue;
			}
		}
	}
	return eContinue;
}

bool CheckUninitVar::CheckMemUseInFun(const Variable &var, const Function* fc, Alloc &alloc)
{
	bGoInner = false;
	bool ret = checkScopeForVariable(fc->functionScope->classStart, var, nullptr, nullptr, &alloc, emptyString, var.isPointer());
	bGoInner = true;
	return ret;
}