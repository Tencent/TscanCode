class C
{
	ObjectCheck Demo(Type type)
	{	
		//使用lamba表达式可能影响性能
		return(RealStatePtr L,int idx) => 
		{    
			if(LuaAPI.lua_type(L,idx) == LuaTypes.LUA_TUSEDATA)
			{
				object obj = translator.getRawNetObject(L,idx);
				returan obj != null && type.IsAssignableFrom(obj.GetType());
			}
			return false;
		};
	}
}