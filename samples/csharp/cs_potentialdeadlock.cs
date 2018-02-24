class Demo{
	private static object staticObjLockA = new Object();
    private static object staticObjLockB = new Object();
	private static object staticObjLockC = new Object();
	//对象A.C.B加了锁
	public void A()	{
		lock(staticObjLockA){
			lock(staticObjLockC){
				AInner();
			}
		}
	}
	//对象B.A加了锁，与A函数一起使用，可能造成死锁
	public void B()	{
		lock(staticObjLockB){
			BInner();
		}
	}
	
	public void AInner(){
		lock(staticObjLockB){}
	}
	public void BInner(){
		lock(staticObjLockA){}
	}
}