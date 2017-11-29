class C
{
	public void M2()
	{
		Mutex m1 = new Mutex();
		Mutex m2 = new Mutex();
		m1.ReleaseMutex();
		m2.WaitOne();           //m2没有释放
	}
}