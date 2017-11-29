GetCookie = luaEnv.GetInPath<xxx>("GetCookie");
ck = GetCookie("name");
//lua 的GetCookie函数可能返回null，c#中没有判定
ck.getExpiredDate(); 