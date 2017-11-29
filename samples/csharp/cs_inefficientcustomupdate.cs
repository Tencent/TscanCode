class C : MonoBehaviour
{
	public Update() 
	{
		//Unity项目中，自定义的Update函数也应该避免使用字符串拼接
		string str = "abc" + "\n"; 
	}
}