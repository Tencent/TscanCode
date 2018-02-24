namespace TscSharp_Samples
{
    enum MyEnum
    {
        A,
        B,
        C,
    }
    class C
    {
        public void Func()
        {
            MyEnum e = MyEnum.A;
            //枚举变量不应该作为key，这会影响性能
            Dictionary<MyEnum, int> dic = new Dictionary<MyEnum, int>();  
        }
    }
}