using XLua;
namespace Assets.Scripts.Logic.Battle {
    public static class BattleUtility {
        private static ProfileDelegate _profile = null;

        public static void Initialize(LuaEnv lua)  {
            //ProfileReport函数没有定义，直接访问
            _profile = lua.Global.GetInPath<ProfileDelegate>("BattleUtility.ProfileReport");
        }

        public static void LuaProfileReport()        {
            _profile(); 
        }
    }
}
