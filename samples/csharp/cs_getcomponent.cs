class C
{
	void Demo()
	{
		point.transform.parent = _fightSectionsTransform;
		//建议保留一份point.transform, 然后使用trans.parent=xxx
		point.transform.localPosition = mapLineInfo.SectionLocalPositions[i];
	}
}