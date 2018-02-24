class C
{
	void Demo()
	{
		if (NetworkModule.GetInstance().CanSendMsgFlag)
		{
			string exceptionFile = string.Format("{0}/makeexception", Application.persistentDataPath);
			if (File.Exists(exceptionFile))
			{
				string exception = null;
				//exception 是null
				exception.IndexOf('e');
			}
		}
	}
}