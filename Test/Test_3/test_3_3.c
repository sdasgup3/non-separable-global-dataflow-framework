#include<stdio.h>
void test()
{
int a,b,c,d;

	d = 0;
	while(d >= 3) {
		if(d >= 2 ) {
			a=b;
		} else {
			if(d >= 1) {
				b=c;
			} else {
			    c = 9;
			}
		}
		d = d +1;
	}	
	if(a >1) { 
	    printf("dsada");
    } 
}
