class C:MonoBehaviour
{
	public Update() 
	{
		//Unity项目中，不应该在Update函数中使用foreach, 这回导致GC
		foreach (var i in m_list)
		{
		}
	}
}