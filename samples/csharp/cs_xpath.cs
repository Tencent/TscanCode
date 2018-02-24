class C
{
	public void Demo()
	{
		System.Xml.XmlDocument document = new XmlDocument();
		//检查XPATH的语法，详情请参考https://msdn.microsoft.com/en-us/library/ms256086(v=vs.110).aspx
		XmlNodeList nodes = document.SelectNodes("/book[@npages== 100]/@title");
	}
}