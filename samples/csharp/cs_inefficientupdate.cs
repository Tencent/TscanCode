class C : MonoBehaviour
{
	public Update() 
	{
		//Unity项目中，在update函数中使用字符串拼接会影响性能
		string str = "abc" + "\n"; 
	}
}