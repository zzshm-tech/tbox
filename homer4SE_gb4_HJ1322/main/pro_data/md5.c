
/*******************************
**
**
********************************/

#include <stdio.h>
#include <string.h>


#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

/**************************/

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/*******************************************
**	
**	
**********************************************/



//
//#pragma udata bank12=0xC00
unsigned int  x[16];				//
unsigned int  a,b,c,d;			//
unsigned int  state[4];			//

/***********************************************
**
**
***********************************************/

unsigned int f(unsigned int x,unsigned int y,unsigned int z)
{
	return (x&y)|((~x)&z);
}

unsigned int g(unsigned int x,unsigned int y,unsigned int z)
{
	return (x&z)|(y&(~z));
}

/**************************
**
**
***************************/
unsigned int h(unsigned int x,unsigned  int y,unsigned int z)
{
	return x^y^z;
}

/*******************************
**
**
*********************************/

unsigned int i(unsigned int x,unsigned int y,unsigned int z)
{
	return y^(x|(~z));
}

/*****************************
**
**	
*****************************/

void ff(unsigned int x,unsigned int s,unsigned int ac)
{
	if(s==S11||s==S21||s==S31||s==S41)
	{
		a+=f(b,c,d)+x+ac;
		a=ROTATE_LEFT(a,s);
		a+=b;
	}
	if(s==S12||s==S22||s==S32||s==S42)
	{
		d+=f(a,b,c)+x+ac;
		d=ROTATE_LEFT(d,s);
		d+=a;
	}        
	if(s==S13||s==S23||s==S33||s==S43)
	{
		c+=f(d,a,b)+x+ac;
		c=ROTATE_LEFT(c,s);
		c+=d;
	}        
	if(s==S14||s==S24||s==S34||s==S44)
	{
		b+=f(c,d,a)+x+ac;
		b=ROTATE_LEFT(b,s);
		b+=c;
	}            
}

/*****************************
**
**	
******************************/
void gg(unsigned int x,unsigned int s,unsigned int ac)
{
	if(s==S11||s==S21||s==S31||s==S41)
	{
		a+=g(b,c,d)+x+ac;
		a=ROTATE_LEFT(a,s);
		a+=b;
	}
	if(s==S12||s==S22||s==S32||s==S42)
	{
		d+=g(a,b,c)+x+ac;
		d=ROTATE_LEFT(d,s);
		d+=a;
	}        
	if(s==S13||s==S23||s==S33||s==S43)
	{
		c+=g(d,a,b)+x+ac;
		c=ROTATE_LEFT(c,s);
		c+=d;
	}        
	if(s==S14||s==S24||s==S34||s==S44)
	{
		b+=g(c,d,a)+x+ac;
		b=ROTATE_LEFT(b,s);
		b+=c;
	}
}

/***************************
**
**
****************************/
void hh(unsigned int x,unsigned int s,unsigned int ac)
{
	if(s==S11||s==S21||s==S31||s==S41)
	{
		a+=h(b,c,d)+x+ac;
		a=ROTATE_LEFT(a,s);
		a+=b;
	}
	if(s==S12||s==S22||s==S32||s==S42)
	{
		d+=h(a,b,c)+x+ac;
		d=ROTATE_LEFT(d,s);
		d+=a;
	}        
	if(s==S13||s==S23||s==S33||s==S43)
	{
		c+=h(d,a,b)+x+ac;
		c=ROTATE_LEFT(c,s);
		c+=d;
	}        
	if(s==S14||s==S24||s==S34||s==S44)
	{
		b+=h(c,d,a)+x+ac;
		b=ROTATE_LEFT(b,s);
		b+=c;
	}
}

/*************************
**
**	
**************************/
void ii(unsigned int x,unsigned int s,unsigned int ac)
{
	if(s==S11||s==S21||s==S31||s==S41)
	{
		a+=i(b,c,d)+x+ac;
		a=ROTATE_LEFT(a,s);
		a+=b;
	}
	if(s==S12||s==S22||s==S32||s==S42)
	{
		d+=i(a,b,c)+x+ac;
		d=ROTATE_LEFT(d,s);
		d+=a;
	}        
	if(s==S13||s==S23||s==S33||s==S43)
	{
		c+=i(d,a,b)+x+ac;
		c=ROTATE_LEFT(c,s);
		c+=d;
	}        
	if(s==S14||s==S24||s==S34||s==S44)
	{
		b+=i(c,d,a)+x+ac;
		b=ROTATE_LEFT(b,s);
		b+=c;
	}
}

/*********************************************
**
**
*********************************************/

void md5_ClearX(void)
{
	unsigned char i;

	for(i=0;i<16;i++)
	{
		x[i]=0x00000000;
	}
}

/***************************************
**
**
***************************************/

void ByteToWord(unsigned char *ptr,unsigned char n)
{
	unsigned char i;
	unsigned char *pTmp;
	unsigned int ls1_32;
	//INT32U xdata tmp=0x00000000;

	pTmp=ptr;

	for(i=0;i<n;i++)
	{
		x[i]=0x00000000;

		x[i]=*pTmp++;
		ls1_32=*pTmp++;
		ls1_32=ls1_32<<8;
		x[i]=x[i]+ls1_32;
		ls1_32=*pTmp++;
		ls1_32=ls1_32<<16;
		x[i]=x[i]+ls1_32;
		ls1_32=*pTmp++;
		ls1_32=ls1_32<<24;
		x[i]=x[i]+ls1_32;


/*//???
for(j=0;j<4;j++)
	{
	x[i]=x[i]<<8;
	x[i]+=*pTmp;
	pTmp++;
	}
*/
	}
}

/**********************************************
**	
**	
**********************************************/

void md5_ProChunk(void)
{
	a=state[0];
	b=state[1];
	c=state[2];
	d=state[3];

	/* ????? */
	ff(x[ 0], S11, 0xd76aa478); /* 1 */
	ff(x[ 1], S12, 0xe8c7b756); /* 2 */
	ff(x[ 2], S13, 0x242070db); /* 3 */
	ff(x[ 3], S14, 0xc1bdceee); /* 4 */
	ff(x[ 4], S11, 0xf57c0faf); /* 5 */
	ff(x[ 5], S12, 0x4787c62a); /* 6 */
	ff(x[ 6], S13, 0xa8304613); /* 7 */
	ff(x[ 7], S14, 0xfd469501); /* 8 */
	ff(x[ 8], S11, 0x698098d8); /* 9 */
	ff(x[ 9], S12, 0x8b44f7af); /* 10 */
	ff(x[10], S13, 0xffff5bb1); /* 11 */
	ff(x[11], S14, 0x895cd7be); /* 12 */
	ff(x[12], S11, 0x6b901122); /* 13 */
	ff(x[13], S12, 0xfd987193); /* 14 */
	ff(x[14], S13, 0xa679438e); /* 15 */
	ff(x[15], S14, 0x49b40821); /* 16 */

	/* ????? */
	gg(x[ 1], S21, 0xf61e2562); /* 17 */
	gg(x[ 6], S22, 0xc040b340); /* 18 */
	gg(x[11], S23, 0x265e5a51); /* 19 */
	gg(x[ 0], S24, 0xe9b6c7aa); /* 20 */
	gg(x[ 5], S21, 0xd62f105d); /* 21 */
	gg(x[10], S22,  0x2441453); /* 22 */
	gg(x[15], S23, 0xd8a1e681); /* 23 */
	gg(x[ 4], S24, 0xe7d3fbc8); /* 24 */
	gg(x[ 9], S21, 0x21e1cde6); /* 25 */
	gg(x[14], S22, 0xc33707d6); /* 26 */
	gg(x[ 3], S23, 0xf4d50d87); /* 27 */
	gg(x[ 8], S24, 0x455a14ed); /* 28 */
	gg(x[13], S21, 0xa9e3e905); /* 29 */
	gg(x[ 2], S22, 0xfcefa3f8); /* 30 */
	gg(x[ 7], S23, 0x676f02d9); /* 31 */
	gg(x[12], S24, 0x8d2a4c8a); /* 32 */

	/* ????? */
	hh(x[ 5], S31, 0xfffa3942); /* 33 */
	hh(x[ 8], S32, 0x8771f681); /* 34 */
	hh(x[11], S33, 0x6d9d6122); /* 35 */
	hh(x[14], S34, 0xfde5380c); /* 36 */
	hh(x[ 1], S31, 0xa4beea44); /* 37 */
	hh(x[ 4], S32, 0x4bdecfa9); /* 38 */
	hh(x[ 7], S33, 0xf6bb4b60); /* 39 */
	hh(x[10], S34, 0xbebfbc70); /* 40 */
	hh(x[13], S31, 0x289b7ec6); /* 41 */
	hh(x[ 0], S32, 0xeaa127fa); /* 42 */
	hh(x[ 3], S33, 0xd4ef3085); /* 43 */
	hh(x[ 6], S34,  0x4881d05); /* 44 */
	hh(x[ 9], S31, 0xd9d4d039); /* 45 */
	hh(x[12], S32, 0xe6db99e5); /* 46 */
	hh(x[15], S33, 0x1fa27cf8); /* 47 */ 
	hh(x[ 2], S34, 0xc4ac5665); /*48 */

	/* ????? */
	ii(x[ 0], S41, 0xf4292244); /* 49 */
	ii(x[ 7], S42, 0x432aff97); /* 50 */
	ii(x[14], S43, 0xab9423a7); /* 51 */
	ii(x[ 5], S44, 0xfc93a039); /* 52 */
	ii(x[12], S41, 0x655b59c3); /* 53 */
	ii(x[ 3], S42, 0x8f0ccc92); /* 54 */
	ii(x[10], S43, 0xffeff47d); /* 55 */
	ii(x[ 1], S44, 0x85845dd1); /* 56 */
	ii(x[ 8], S41, 0x6fa87e4f); /* 57 */
	ii(x[15], S42, 0xfe2ce6e0); /* 58 */
	ii(x[ 6], S43, 0xa3014314); /* 59 */
	ii(x[13], S44, 0x4e0811a1); /* 60 */
	ii(x[ 4], S41, 0xf7537e82); /* 61 */
	ii(x[11], S42, 0xbd3af235); /* 62 */
	ii(x[ 2], S43, 0x2ad7d2bb); /* 63 */
	ii(x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}



/**********************************
**	
**
**********************************/

void md5(unsigned  char* ptr,unsigned int *pState,unsigned int len)
{
	unsigned int len1;
	unsigned char * pTmp;
	unsigned char n,m;
	unsigned char ls1;

	pTmp=ptr;
	//len=8;//???????
	len1=len;

/*
state[0]=0x01234567;
state[1]=0x89abcdef;
state[2]=0xfedcba98;
state[3]=0x76543210;
*/

	state[0]=0x67452301;
	state[1]=0xefcdab89;
	state[2]=0x98badcfe;
	state[3]=0x10325476;

// ??0:????????????
	while((len / 64) > 0)
	{  
		md5_ClearX();//??ByteToWord()???????,?????x[]?????????,???????
		ByteToWord(pTmp,16);//??????x[]?
		md5_ProChunk();//????
		len -= 64;//????????,?????????????64 Byte(512 bit)
		pTmp += 64;//???????
}
// ??0

// ??1:???????????
	md5_ClearX();
	ls1=len / 4;
	ByteToWord(pTmp,ls1);//?????????x[]

// ??1.0 ?????????????????????x[]
	n = len % 4;
	m = len / 4;
	switch(n)
	{
		case 0:
			x[m] = x[m]|0x00000080;//??0?????
			break;
		case 1:
			x[m] = *pTmp;
			x[m] = x[m]|0x00008000;//??1?????
			break;
		case 2:
			x[m] = *(pTmp+1);
			x[m] = x[m]<<8;
			x[m] = x[m]+(*pTmp);
			x[m] = x[m]|0x00800000;//??2?????
			break;
		case 3:
			x[m]=*(pTmp+2);
			x[m]=x[m]<<8;
			x[m]=x[m]+*(pTmp+1);
			x[m]=x[m]<<8;
			x[m]=x[m]+(*pTmp);
			x[m]=x[m]|0x80000000;//??3?????
		default:
			break;
	}
// ??1.0

	if(len >= 56)//???????????56?????????
	{
		md5_ProChunk();
		md5_ClearX();
	}

	x[14] = len1*8;//?????‘?’????
	//x[15]..
	md5_ProChunk();
	// ??1
	pTmp = (unsigned char *)pState;
	for(n = 0;n < 4;n++)
	{
		memcpy(pTmp + n * 4,(unsigned char *)&state[n],4);
	}
}


/*******************************
**	º¯ÊýÃû³Æ£º
**	¹¦ÄÜÃèÊö:
********************************/

/*
void md5_seed(void)	//??ECU??SEED??MD5???
{
	unsigned char seed[8];
	
	seed[0]=0xA7;
	seed[1]=0x6F;
	seed[2]=0x3B;
	seed[3]=can_rx_data[0];
	seed[4]=can_rx_data[1];
	seed[5]=can_rx_data[2];
	seed[6]=can_rx_data[3];
	seed[7]=can_rx_data[4];

	cansend_data.data[0]=0xA7;
	cansend_data.data[1]=0x6F;
	cansend_data.data[2]=0x3B;
	md5(seed);	//??MOD5?
	//?????????state??
	//????
	//seed=33556699cc6633c1
	//md5=08acd941a1d9f524c552e5948f054df7
	//state[0]=0x41D9AC08
	//state[1]=0x24F5D9A1
	//GPS????:A76F3B
	cansend_data.id=0x180001FB;	//??ID

	cansend_data.data[0]=state[0];
	state[0]=state[0]>>8;
	cansend_data.data[1]=state[0];
	state[0]=state[0]>>8;
	cansend_data.data[2]=state[0];
	state[0]=state[0]>>8;
	cansend_data.data[3]=state[0];

	cansend_data.data[4]=state[1];
	state[1]=state[1]>>8;
	cansend_data.data[5]=state[1];
	state[1]=state[1]>>8;
	cansend_data.data[6]=state[1];
	state[1]=state[1]>>8;
	cansend_data.data[7]=state[1];
	can_tx_onedata();
	return;
	}
*/


/*
void md5test(void)
{
	unsigned char test[8];

	test[0]=0X44;
	test[1]=0X55;
	test[2]=0X66;
	test[3]=0X99;
	test[4]=0XCC;
	test[5]=0X66;
	test[6]=0X33;
	test[7]=0XC1;
	
	¡¢¡¢md5(test);
	
	}
*/

	
	/**************File End*******************/
	
	
	
	
