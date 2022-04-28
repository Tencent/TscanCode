# Change Log
All notable changes of this project will be documented here.
## 2022-04-28
* one crash fix and false positive issues fixes
* more checkers opened to public, see lua/cfg/lua_cfg.xml for detail
## 2022-01-20
* support lua5.4
* enhanced support for require/_G
* enhanced lua_cfg.xml
* more smart logical reasoning, 
```
local function UninitVar_FP24(charactor) 
    local controller 
    if slua.isValid(charactor) then 
        controller = 1 
    end 

    local uBack 
    if slua.isValid(controller) then 
        uBack = 2 
    end 

    if not slua.isValid(uBack) then 
        return 
    end 
 
    return controller + 1 --no error 
end
```

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
