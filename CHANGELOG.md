# Change Log
All notable changes of this project will be documented here.

## 2.14.24 - 2018-02-24

### Update List
* Rule Package was released on GUI, easier for rule customization;
* GUI supports marking false-positive errors now.
* Fixed known false positives and false negatives of null-pointer, buffer-overrun checks and so on;
* Refined error messages for several checks;
* Known GUI bugs fixed;

## 2.11.27 - 2017-11-30

### Added
* Add new check `CS_UnityMessgeSpellWrong`, check typo errors of Unity message functions, e.g. `OnDestroy` is misspelled as `OnDestory`;
* Add new check `lua_VarSpellWrongError` and `lua_KeywordSpellWrongError`, check typo errors of Lua variable and keyword, e.g. `false` is misspelled as `flase`;

### Fixed
Several known bugs fixed.
