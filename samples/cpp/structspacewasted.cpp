// 在4字节对齐的情况下，sizeof(A)的大小是12字节，调换字段b和c的位置A的大小可减少为8字节
struct A
{
	char a;
	int b;
	char c;
};