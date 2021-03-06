
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <amosKittens.h>
#endif

#ifdef __linux__
#include <limits.h>
#endif

#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "debug.h"
#include "kittyErrors.h"
#include "amosString.h"

extern unsigned short last_token;
extern int tokenMode;
extern int current_screen;

extern struct retroScreen *screens[8] ;

extern void setStackStr( struct stringData *str );
extern void setStackStrDup( struct stringData *str );

using namespace std;


/*********

string names like _xxx is read only (const char), and need to copied.
string names like xxx is new and can saved on stack, with out being copied.

*********/



char *_instr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1 ;
	struct stringData *_str,*_find;
	int  _pos = 0;
	int _start = 0;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
				_str = getStackString(__stack - 1 );
				_find = getStackString(__stack );

				if ((_str)&&(_find))
				{
					if ((_str -> size) &&(_find -> size))		// not empty
					{
						_pos = amos_instr( _str, 0, _find );
					}
				}
				break;
		case 3:
				_str = getStackString(__stack - 2 );
				_find = getStackString(__stack -1 );
				_start = getStackNum(__stack ) -1;

				if ((_str)&&(_find)&&(_start>-1) )
				{
					if ((_str -> size) &&(_find -> size))		// not empty
					{
						int str_len = kittyStack[__stack-2].str -> size;

						if (_start >= str_len) _start = str_len-1;

						_pos = amos_instr( _str , _start, _find );
					}
				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}	

	popStack(__stack - data->stack);

	setStackNum( _pos );

	return NULL;
}

char *_cmdStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				{
					struct stringData *_str = alloc_amos_string( 50 );
					int num;
					num = getStackNum(__stack );

					if (_str)
					{
						if (num>-1)
							sprintf(&_str->ptr," %d",num);
						else
							sprintf(&_str->ptr,"%d",num);
	
						_str->size = strlen(&_str->ptr);				
						setStackStr(_str);
						return NULL;
					}
				}
				break;
	}

	popStack(__stack - data->stack);
	setError(22,data->tokenBuffer);
	return NULL;
}




char *_asc( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1 ;
	struct stringData *_str;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		_str = getStackString(__stack  );
		if (_str) ret = _str -> ptr;
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack);

	setStackNum( ret );

	return NULL;
}


static bool get_bin(char *str, int &num)
{
	char *c = str;
	num = 0;

	while (*c == ' ') c++;
	if (*c!='%') return false;
	c++;
	while ((*c=='0')||(*c=='1'))
	{
		num = num << 1;
		num += *c - '0';
		c++;
	}

	return true;
}

static bool get_hex(char *str, int &num)
{
	char *c = str;
	num = 0;

	while (*c == ' ') c++;
	if (*c!='$') return false;
	c++;
	while ( (*c) &&  (*c != ' ')  )
	{
		num = num * 16;

		if ((*c>='0')&&(*c<='9'))
		{
			num += *c -'0';
		}
		else if ((*c>='a')&&(*c<='f'))
		{
			num += *c -'a' +10;
		}
		else if ((*c>='A')&&(*c<='F'))
		{
			num += *c -'A' +10;
		}
		else break;

		c++;
	}

	return true;
}


char *_val( struct glueCommands *data, int nextToken )
{
	int num = 0;
	double numf = 0.0f;
	char *c;
	struct stringData *_str;
	int type_count = 0;
	int type = 0;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_str = getStackString(__stack  );
	if (_str)
	{
		c = &_str -> ptr;
		// skip spaces in the start of the string.
		while (*c == ' ') c++;

		// check for symbol until first space or end of string.
		for (; (*c)  && (*c != ' ') ;c++)
		{
			switch (*c)
			{
				case '.':	type= 1; type_count++; break;
				case '$':	type= 2; type_count++; break;
				case '%':	type= 3; type_count++; break;
			}
		}	

		if (type_count<2)
		{
			success = true;

			switch (type)
			{
				case 0:	if (sscanf(&_str -> ptr,"%d",&num)==0) num=0.0f;
						break;
				case 1:	if (sscanf(&_str -> ptr,"%lf",&numf)==0) numf=0.0f;
						break;
				case 2:	success = get_hex(&_str -> ptr,num);
						break;
				case 3:	success = get_bin(&_str -> ptr,num) ;
						break;

				default: success = false;
			}
		}
		else printf("type_count %d\n",type_count);
	}

	popStack(__stack - data->stack);

	if (success == true)
	{
		if (type == 1)
		{
			setStackDecimal( numf );
		}
		else
		{
			setStackNum( num );
		}
	}
	else
	{
		setError(22, data -> tokenBuffer);
	}

	return NULL;
}


char *_len( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1 ;
	int len = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			if (kittyStack[__stack].type == type_string)
			{
				len  = kittyStack[__stack].str -> size;
				setStackNum( len );
				return NULL;
			}

			setError(22,data->tokenBuffer);
			break;

		default:
			popStack(__stack - data->stack);
			setError(22,data->tokenBuffer);
	}

	return NULL;
}


char *_cmdLeftStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
			str = getStackString(__stack - 1 );
			_len = getStackNum(__stack );
			if (_len>-1) tmp = amos_strndup(str, _len );
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - data->stack);
			return NULL;
	}

	popStack(__stack - data->stack);
	if (tmp) 
	{
		setStackStr(tmp);
	}
	else setError(23,data->tokenBuffer);

	return NULL;
}

char *cmdLeftStr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _cmdLeftStr, ptr );
	return ptr;
}

char *_cmdMidStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _start=0, _len = 0;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
			str = getStackString(__stack - 1 );
			_start = getStackNum(__stack ) ;	
			_len = (str) ? str -> size : 0;
			break;

		case 3:
			str = getStackString(__stack - 2 );
			_start = getStackNum(__stack -1 )  ;
			_len = getStackNum(__stack );
			break;

		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - data->stack);
			return NULL;
	}

	if (_start<0)
	{
		popStack(__stack - data->stack);
		setError(23,data->tokenBuffer);
		return NULL;
	}
	else
	{
		if (_start) _start --;
		if (_start>str -> size) 
		{
			tmp = toAmosString("",0);
		}
		else
		{
			if ( (_start+_len) > str->size) _len = str->size- _start;
			tmp = amos_mid(str, _start, _len );
		}	
		popStack(__stack - data->stack);
		if (tmp) setStackStr(tmp);
	}
	return NULL;
}

char *cmdMidStr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _cmdMidStr, ptr );
	return ptr;
}

char *_cmdRightStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
			str = getStackString(__stack - 1 );
			_len = getStackNum(__stack  );

			if (_len>-1)	// success
			{
				if (_len>str->size) _len = str ->size;
				tmp = amos_right(str , _len );
				break;
			}
			else	// failed
			{
				setError(23,data->tokenBuffer);
				popStack(__stack - data->stack);
			}
			return NULL;

		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - data->stack);
			return NULL;
	}

	popStack(__stack - data->stack);
	if (tmp) 
	{
		setStackStr(tmp);
	}
	else setError(23,data->tokenBuffer);

	return NULL;
}

char *cmdRightStr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _cmdRightStr, ptr );
	return ptr;
}

char *_cmdHexStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	int num,chars;
	char fmt[10];
	struct stringData *_str = alloc_amos_string( 50 );

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				num = getStackNum(__stack );
				sprintf(&_str->ptr,"$%X",num);
				break;
		case 2:
				num = getStackNum(__stack-1 );
				chars = getStackNum(__stack );	
				sprintf(fmt,"$%%0%dX",chars);
				sprintf(&_str->ptr,fmt,num);
				break;
	}

	popStack(__stack - data->stack);

	_str -> size = strlen( &_str->ptr );
	setStackStr(_str);

	return NULL;
}

char *cmdHexStr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _cmdHexStr, ptr );
	return ptr;
}

char *_cmdBinStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1;
	unsigned int num= 0,len =0,n;
	struct stringData *str;
	char *p;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:	num = getStackNum(__stack  );
				break;
		case 2:	num = getStackNum(__stack -1 );
				len = getStackNum(__stack );
				break;
		default: 
				setError(22,data->tokenBuffer);
	}
	popStack(__stack - data->stack);

	if (args == 1)
	{
		len = 0;
		for (n= num ; n!=0 ; n >>= 1 ) len++;
		len = len ? len : len + 1;	// always one number in bin number.
	}

	str = alloc_amos_string(len+1);	 //  '%' 

	if (str)
	{
		p = &(str -> ptr);

		*p++='%';

		for (n=len;n>0;n--)
		{
			*p++= (num & (1<<(n-1))) ? '1' : '0';
		}
		*p = 0;
	}

	str -> size = strlen( &str->ptr );
	setStackStr(str);

	return NULL;
}

char *cmdBinStr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _cmdBinStr, ptr );
	return ptr;
}

char *cmdInstr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _instr, ptr );
	return ptr;
}

char *_cmdFlipStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	int l,i;
	struct stringData *_str;
	char *str,t;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 1)
	{
		_str = getStackString(__stack  );

		if (_str)
		{
			l = _str -> size;
			str = &_str -> ptr;

			for (i=0;i<l/2;i++)
			{
			 	t = str[i] ;
				str[i] = str[l-1-i];
				str[l-1-i] = t;
			}
		}
	}
	else
	{
		popStack(__stack - data->stack);
		setError(22, data -> tokenBuffer);
	}

	return NULL;
}

char *cmdFlipStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdFlipStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *_cmdSpaceStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int i,_len = getStackNum(__stack );
				struct stringData *str = alloc_amos_string(_len);
				char *p;

				if (str)
				{
					p = &str -> ptr;
					for (i=0;i<_len;i++) *p++=' ';
					*p= 0;
					setStackStr(str);
					return NULL;		// cool success return...
				}
			}

			printf("new string to allocate with %d bytes\n", _len );

			setError(22 , data -> tokenBuffer);	// failed to allocate mem...	
			return NULL;

		default:
			popStack(__stack - data->stack);
			setError(22 , data -> tokenBuffer);
	}

	return NULL;
}

char *cmdSpaceStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdSpaceStr, tokenBuffer );	// we need to store the step counter.
	setStackNone();
	return tokenBuffer;
}

char *_cmdUpperStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1;
	struct stringData *str;
	char *s;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args != 1)
	{
		popStack(__stack - data->stack);
		setError(22 , data -> tokenBuffer);
		return NULL;
	}

	str = getStackString(__stack );
	if (str)
	{
		for (s=&str -> ptr;*s;s++) if ((*s>='a')&&(*s<='z')) *s+=('A'-'a');
	}

	return NULL;
}

char *cmdUpperStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdUpperStr, tokenBuffer );
	return tokenBuffer;
}

char *_cmdLowerStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1;
	struct stringData *str;
	char *s;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args != 1)
	{
		popStack(__stack - data->stack);
		setError(22 , data -> tokenBuffer);
		return NULL;
	}

	str = getStackString(__stack );
	if (str)
	{
		for (s=&str ->ptr;*s;s++) if ((*s>='A')&&(*s<='Z')) *s-=('A'-'a');
	}

	return NULL;
}

char *cmdLowerStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdLowerStr, tokenBuffer );
	return tokenBuffer;
}

char *_cmdStringStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	int i,_len;
	struct stringData *str = NULL;
	struct stringData *_str;
	char *dest;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			_str = getStackString(__stack - 1 );
			_len = getStackNum(__stack  );

			str = alloc_amos_string(_len);
			
			dest = &str -> ptr;

			for (i=0;i<_len;i++)
			{
				dest[i]= (_str ? _str -> ptr : 0) ;
			}
			dest[i]= 0;

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack);
	if (str) setStackStr(str);

	return NULL;
}


char *cmdStringStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdStringStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *_cmdChrStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack + 1;
	struct stringData *_str = alloc_amos_string( 1 );
	char *p;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		if (_str)
		{
			p = &(_str -> ptr);
			*p++ = args == 1 ? (char) getStackNum(__stack ) : 0;
			*p = 0;
		}
		setStackStr(_str);
		return NULL;
	}

	setError(22,data -> tokenBuffer);
	popStack(__stack - data->stack);
	setStackStr(_str);

	return NULL;
}

char *cmdChrStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdChrStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdAsc(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _asc, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdLen(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _len, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdVal(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _val, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

void	_match_int( struct kittyData *array, int value )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	int delta =INT_MAX;
	struct valueData *ptr = &(array -> int_array -> ptr);

	for (n =0; n< array -> count; n++)
	{
		new_delta = abs( ptr[n].value - value) ;

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				setStackNum(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	setStackNum( -closest );
}

void _match_float( struct kittyData *array, double decimal )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	double delta =INT_MAX;	// yes I know double supports larger numbers, but this should work here.
	struct desimalData *ptr = &(array -> float_array -> ptr);

	for (n =0; n< array -> count; n++)
	{
		new_delta = ptr[n].value - decimal ;
		new_delta = new_delta < 0.0f ? -new_delta : new_delta;	// abs() but double.

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				setStackNum(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	setStackNum( -closest );
}

void _match_str( struct kittyData *array,  struct stringData *strArg )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	int found_chars = 0;
	int delta =INT_MAX;
	char *str = &(strArg -> ptr);
	int i;

	int _l = strArg->size;

	char c;
	struct stringData **item = &array -> str_array -> ptr; 
	char *itemStr;

	for (n =0; n< array -> count; n++)
	{
		itemStr = &(item[n] -> ptr);

		if (itemStr)
		{
			found_chars = 0;

			for ( i=0;i<_l;i++)
			{
				c = itemStr[i];
				if (c == 0) break;
				if (c == str[i]) found_chars++;
			}

			new_delta = abs( max( _l , (int) strlen( itemStr ) )  - found_chars ) ;

			if ( new_delta < delta )
			{
				if (new_delta == 0)
				{
					setStackNum( n );
					return;
				}
				else
				{
					delta = new_delta;
					closest = n;
				}
			}
		}
	}

	setStackNum( -closest );
}

void	sort_int_array(	struct kittyData *var )
{
	bool sorted = FALSE;
	int n,v;
	struct valueData *i0,*i1;

	do
	{
		sorted = false;
		i0 = &var -> int_array -> ptr; i1 = i0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( i0 -> value > i1 -> value  )
			{
				v = i0 -> value; i0 -> value = i1 -> value; i1 -> value = v;
				sorted = true;
			}

			i0++; i1++;
		}
	} while (sorted);
}

void	sort_float_array( struct kittyData *var )
{
	bool sorted = FALSE;
	int n;
	struct desimalData v;
	struct desimalData *f0;
	struct desimalData *f1;

	do
	{
		sorted = false;
		f0 = &var -> float_array -> ptr; f1 = f0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( f0 -> value > f1 -> value  )
			{
				v = *f0; *f0 = *f1; *f1 = v;
				sorted = true;
			}

			f0++; f1++;
		}
	} while (sorted);
}

void	sort_string_array( struct kittyData *var )
{
	bool sorted = FALSE;
	int n;
	struct stringData *v;
	struct stringData **s0,**s1;

	do
	{
		sorted = false;
		s0 = &var -> str_array -> ptr; s1 = s0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( strcmp( &(*s0)->ptr, &(*s1)->ptr ) > 0  )
			{
				v = *s0; 
				*s0 = *s1; 
				*s1 = v;

				sorted = true;
			}
			s0++; s1++;
		}
	} while (sorted);
}


// AMOS The Creator User Guide states that array has to be at index 0, 
// so we don't need to read tokens after variable name,
// we can process this direcly no callbacks.|

char *cmdSort(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = *((short *) (tokenBuffer));

	struct reference *ref = (struct reference *) (tokenBuffer + 2);

	if (next_token == 0x0006)
	{
		struct kittyData *var = getVar( ref -> ref);

		if (var -> type & type_array)	// is array
		{
			switch (var -> type & 7)
			{
				case type_int:
					sort_int_array(var);
					break;
				case type_float:
					sort_float_array(var);
					break;
				case type_string:
					sort_string_array(var);	
					break;
			}
		}		

		tokenBuffer += 2 + sizeof( struct reference) + ref -> length;
	}

	stackCmdParm( _cmdStr, tokenBuffer );	// we need to store the step counter.

	return tokenBuffer;
}

#define badSyntax() { setError(120,tokenBuffer); return NULL; }

char *cmdMatch(struct nativeCommand *cmd, char *tokenBuffer )
{
	struct reference *ref = NULL;
	struct kittyData *array_var = NULL;
	struct kittyData *var = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) != 0x0074) badSyntax();	// (
	tokenBuffer +=2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// array
	
	ref = (struct reference *) (tokenBuffer + 2);
	array_var = getVar( ref -> ref) ;
	tokenBuffer += sizeof( struct reference) + ref -> length + 2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x0074) badSyntax();	// (
	tokenBuffer +=2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x003E) badSyntax();	// 0
	tokenBuffer += 6;	

	if (NEXT_TOKEN( tokenBuffer ) != 0x007C) badSyntax();	// )
	tokenBuffer +=2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x005C) badSyntax();	// ,
	tokenBuffer +=2;

	// --- this part can be rewritten ---, callback on this part can make sense.
	// just store the array for later...

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// var

	ref = (struct reference *) (tokenBuffer + 2);

	var = getVar( ref -> ref );
	tokenBuffer += 2 + sizeof( struct reference) + ref -> length;

	if (NEXT_TOKEN( tokenBuffer ) != 0x007C) badSyntax();	// )
	tokenBuffer +=2;

	if ((array_var -> type & type_array) && ( (array_var -> type & 7) == var -> type ))
	{
		printf("we are here\n");

		switch (var -> type )
		{
			case type_int:

				printf("int\n");

				_match_int( array_var, var -> integer.value );
				break;

			case type_float:

				printf("float\n");

				_match_float( array_var, var -> decimal.value );
				break;

			case type_string:

				printf("str\n");

				_match_str( array_var, var -> str );
				break;
		}
	}

	return tokenBuffer;
}

char *_cmdRepeatStr( struct glueCommands *data, int nextToken )
{
	string txt;
	int args = __stack - data->stack + 1;
	struct stringData *str;
	struct stringData *dest = NULL;
	int _num,n;
	int _new_size;
	char *d;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = getStackString(__stack - 1 );
		_num = getStackNum(__stack );

		_new_size = _num * str -> size;

		dest = alloc_amos_string( _new_size );
		d =&dest -> ptr;

		for (n=0;n<_new_size;n++)
		{
			d[n] = (&str -> ptr) [ n % str->size ] ;
		}
	}	

	popStack(__stack - data->stack);

	setStackStr(dest);

	return NULL;
}

char *cmdRepeatStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdRepeatStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdTabStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	struct stringData *txt = alloc_amos_string(1);

	if (txt)
	{
		char *p = &txt -> ptr ;
		*p++=9; 
		*p=0;
	}

	setStackStr(txt);
	return tokenBuffer;
}

