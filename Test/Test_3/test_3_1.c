#include<stdio.h>
void test()
{
int a,b,c,d;

	d = 0;
	x:	if(d >= 3) {
		printf("%d",a);
	} else {	
		if(d >= 2 ) {
			a=b;
		} else {
			if(d >= 1) {
				b=c;
			} else {
				scanf("%d",&c);
			}
		}
		d = d +1;
		goto x;
	}

}
