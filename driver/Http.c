#include "Http.h"

int HttpParseRequestLine(PHTTP_REQUEST pHttp);
int httpParseHeaderLine(PHTTP_REQUEST pHttp);

char g_boundarybuffer[200];
int g_boundarybufferlen=0;


HTTP_REQUEST g_httpre = {0};

int MiMeParseFile(PHTTP_REQUEST pHttp);

BOOLEAN HttpHasFileTailByBoundary(PHTTP_REQUEST pHttp)
{
	char end[100] = {0};
	int endlen;
	int bodylen;

	strcat(end, "--");
	strcat(end, pHttp->Boundary);
	strcat(end, "--");
	endlen = strlen(end);
	bodylen = pHttp->Body.End-pHttp->Body.Start;

	if (endlen <= bodylen)
	{
		if (!_strnicmp(end, pHttp->Body.End-endlen, endlen))
		{
			g_boundarybufferlen = 0;
			return TRUE;
		}
		else
		{
			memcpy(g_boundarybuffer, pHttp->Body.End-endlen, endlen);
			g_boundarybufferlen = endlen;
			return FALSE;
		}
	}
	else
	{
		memcpy(g_boundarybuffer+g_boundarybufferlen, pHttp->Body.Start, bodylen);
		g_boundarybufferlen += bodylen;

		if (!_strnicmp(end, g_boundarybuffer, endlen))
		{
			g_boundarybufferlen = 0;
			return TRUE;
		}
		else
		{
			if (g_boundarybufferlen <= endlen)
			{
				;
			}
			else
			{
				RtlMoveMemory(g_boundarybuffer, g_boundarybuffer+(g_boundarybufferlen - endlen), endlen);
				g_boundarybufferlen = endlen;
			}
			
			return FALSE;
		}
	}



	return TRUE;
}

int HttpAnalysis(PVOID pBuffer, INT length)
{
	int iRnt = 0;
	ANSI_STRING temp = {0};
	

	g_httpre.buffer = pBuffer;
	g_httpre.length = length;

	g_httpre.status = 0;
	do 
	{
		switch(g_httpre.step)
		{
		case 0://解析http请求行
			if (HttpParseRequestLine(&g_httpre))
			{
				goto done;
			}
			g_httpre.step = 1;

			/*temp.Buffer = httpre.sMethod.Start;
		temp.MaximumLength = temp.Length = httpre.sMethod.End - httpre.sMethod.Start;
		KdPrint(("Method=%Z ", &temp));

		temp.Buffer = httpre.Uri.Start;
		temp.MaximumLength = temp.Length = httpre.Uri.End - httpre.Uri.Start;
		KdPrint(("Uri=%Z ", &temp));

		KdPrint(("Http Version %d.%d\n", httpre.http_major, httpre.http_minor));*/

			break;
		case 1://解析http请求头
			if (httpParseHeaderLine(&g_httpre))
			{
				goto done;
			}
			g_httpre.step = 2;
			break;

		case 2://
			


			if (g_httpre.FileType == 1)
			{
				KdPrint(("updata file stream\n"));
				goto done;
			}
			else if (g_httpre.FileType == 2)
			{
				KdPrint(("updata file form\n"));
				

				if (g_httpre.Body.End > g_httpre.Body.Start)
				{//有文件内容
					MiMeParseFile(&g_httpre);
				/*	if (g_httpre.FileNumber)
					{
						KdPrint(("have file %d\n", g_httpre.FileNumber));
					}*/
					goto done;
				}
				else
				{//无body内容，下一个包是body
					g_httpre.step = 3;
					goto udone;
				}
			}
			else
			{
				goto done;
			}

			break;
		case 3://包是body
			{
				if (g_httpre.FileType == 1)
				{
					goto done;
				}
				else if (g_httpre.FileType == 2)
				{
					g_httpre.Body.Start = g_httpre.buffer;
					g_httpre.Body.End = g_httpre.buffer+g_httpre.length;


					MiMeParseFile(&g_httpre);
					/*if (g_httpre.FileNumber)
					{
						KdPrint(("have file %d\n", g_httpre.FileNumber));
					}*/

					//if (*g_httpre.Body.Start == '0')
					//{//修改测试
					//	*g_httpre.Body.Start = 'w';
					//	*(g_httpre.Body.Start+1) = 'a';
					//	iRnt = 1;
					//}

					if (HttpHasFileTailByBoundary(&g_httpre))
					{
						goto done;
					}
					else
					{
						goto udone;
					}
				}
				else
				{
					goto done;
				}
				
			}
			break;
		default:
			goto done;
		}

	} while (TRUE);


done:
g_httpre.step = 0;

udone:


	//处理解析出来的文件
	if (g_httpre.FileType == 2)
	{
		int i=0;
		int j=0;
		KdPrint(("BodyStart=%x BodyEnd=%x FileNumber=%d MiMeState=%d\n",
			g_httpre.Body.Start,
			g_httpre.Body.End,
			g_httpre.FileNumber,
			g_httpre.MiMeState));

		j = g_httpre.FileNumber;
		while(i<j)
		{
			KdPrint(("FileName=%s FileBuffer=%x FileBufferEnd=%x\n",
				g_httpre.File[i].FileName,
				g_httpre.File[i].FileBuffer,
				g_httpre.File[i].FileBufferEnd));

			//修改文件内容
			if (/*g_httpre.File[i].FileBuffer < g_httpre.Body.End &&*/
				g_httpre.File[i].FileBuffer < g_httpre.File[i].FileBufferEnd)
			{
				//if (*g_httpre.File[i].FileBuffer == '0')
				//{
				//	//*g_httpre.File[i].FileBuffer = 'w';
				//	//*(g_httpre.File[i].FileBuffer+1) = 'a';

				//	RtlCopyMemory(g_httpre.File[i].FileBuffer, "abcdefjabcdefjabcdefj", strlen("abcdefjabcdefjabcdefj"));
				//	iRnt = 1;
				//}

				KdPrint(("%c to w\n", *g_httpre.File[i].FileBuffer));
				*g_httpre.File[i].FileBuffer = 'w';

				iRnt = 1;
			}



			if (g_httpre.MiMeState == 0)
			{
				ExFreePoolWithTag(g_httpre.File[i].FileName, 0);
				g_httpre.File[i].FileName=0;
				g_httpre.FileNumber--;
			}

			i++;
		}

	}

	return iRnt;
}

int HttpParseRequestLine(PHTTP_REQUEST pHttp)
{
	PCHAR pRequestLine;
	UCHAR ch;
	UCHAR m;
	UCHAR c;

	enum {
		sw_start = 0,
		sw_method,
		sw_spaces_before_uri,
		sw_schema,
		sw_schema_slash,
		sw_schema_slash_slash,
		sw_host_start,
		sw_host,
		sw_host_end,
		sw_host_ip_literal,
		sw_port,
		sw_host_http_09,
		sw_after_slash_in_uri,
		sw_check_uri,
		sw_check_uri_http_09,
		sw_uri,
		sw_http_09,
		sw_http_H,
		sw_http_HT,
		sw_http_HTT,
		sw_http_HTTP,
		sw_first_major_digit,
		sw_major_digit,
		sw_first_minor_digit,
		sw_minor_digit,
		sw_spaces_after_digit,
		sw_almost_done
	} state;

	pHttp->FileType = FALSE;

	pRequestLine = pHttp->buffer;
	state = pHttp->status;
	while(pRequestLine <= pHttp->buffer+pHttp->length)
	{
		ch = *pRequestLine;

		switch (state)
		{
		case sw_start:
			{
				pHttp->RequestLine.Start = pRequestLine;
				if (ch == 0x0a || ch == 0x0d)
				{
					break;
				}

				if ((ch < 'A' || ch > 'Z') && ch != '_') 
				{
					return HTTP_PARSE_INVALID_METHOD;
				}

				state = sw_method;

				break;
			}
		case sw_method:
			{
				if (ch == ' ')
				{
					pHttp->sMethod.End = pRequestLine;
					pHttp->sMethod.Start = pHttp->RequestLine.Start;
					switch(pHttp->sMethod.End - pHttp->sMethod.Start)
					{//提取iMethod
					case 3:
						break;
					case 4:
						break;
					case 5:
						break;
					case 6:
						break;
					case 7:
						break;
					case 8:
						break;
					case 9:
						break;
					}

					state = sw_spaces_before_uri;
					break;
				}

				if ((ch < 'A' || ch > 'Z') && ch != '_') 
				{
					return HTTP_PARSE_INVALID_METHOD;
				}
				break;
			}
		case sw_spaces_before_uri:
			{
				if (ch == '/') 
				{
					pHttp->Uri.Start = pRequestLine;
					state = sw_after_slash_in_uri;
					break;
				}

				c =  (ch | 0x20);
				if (c >= 'a' && c <= 'z') 
				{//uri可能带有host
					state = sw_schema;
					break;
				}

				switch (ch) 
				{
					case ' ':
						break;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_schema:
		case sw_after_slash_in_uri:
			{//暂时不解析uri
				if (ch == ' ')
				{
					pHttp->Uri.End = pRequestLine;
					pRequestLine++;
					state = sw_http_H;
					break;
				}

				break;
			}
		case sw_http_H:
			{
				switch (ch) 
				{
					case 'T':
						state = sw_http_HT;
						break;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HT:
			{
				switch (ch) 
				{
				case 'T':
					state = sw_http_HTT;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTT:
			{
				switch (ch) 
				{
				case 'P':
					state = sw_http_HTTP;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_http_HTTP:
			{
				switch (ch) 
				{
				case '/':
					state = sw_first_major_digit;
					break;
				default:
					return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}
		case sw_first_major_digit:
			{
				if (ch < '1' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_major = ch - '0';
				state = sw_major_digit;
				break;
			}
		case sw_major_digit:
			{
				if (ch == '.') 
				{
					state = sw_first_minor_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

			 	pHttp->http_major = pHttp->http_major * 10 + ch - '0';
				break;
			}
		case sw_first_minor_digit:
			{
				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = ch - '0';
				state = sw_minor_digit;
				break;
			}
		case sw_minor_digit:
			{
				if (ch == 0x0d) 
				{
					state = sw_almost_done;
					break;
				}

				if (ch == 0x0a) 
				{
					goto done;
				}

				if (ch == ' ') 
				{
					state = sw_spaces_after_digit;
					break;
				}

				if (ch < '0' || ch > '9') 
				{
					return HTTP_PARSE_INVALID_REQUEST;
				}

				pHttp->http_minor = pHttp->http_minor * 10 + ch - '0';
				break;
			}
		case sw_spaces_after_digit:
			{
				switch (ch) 
				{
					case ' ':
						break;
					case 0x0d:
						state = sw_almost_done;
						break;
					case 0x0a:
						goto done;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
				break;
			}


		case sw_almost_done:
			{
				pHttp->RequestLine.End = pRequestLine-1;
				switch (ch) 
				{
					case 0x0a:
						goto done;
					default:
						return HTTP_PARSE_INVALID_REQUEST;
				}
			}

		default:
			KdPrint(("unknow\n"));
		}

		pHttp->status = state;
		pRequestLine++;
	}

	return 3;
done:
	pHttp->status = sw_start;
	return 0;
}

int httpMatchName(PUCHAR name, INT length)
{
	int nRtn = 0;

	if (!_strnicmp(name, "Mail-Upload-name", length))
	{
		nRtn = 1;
	}
	else if(!_strnicmp(name, "Content-Type", length))
	{
		nRtn = 2;
	}
	else if (!_strnicmp(name, "Connection", length))
	{
	}
	else if (!_strnicmp(name, "Host", length))
	{
	}
	else
	{
		nRtn = 0;
	}

	return nRtn;
}

int httpMatchValue(PUCHAR name, INT length)
{
	int nRtn = 0;

	if (!_strnicmp(name, "application/octet-stream", strlen("application/octet-stream")))
	{
		nRtn = 1;
	}
	else if(!_strnicmp(name, "multipart/form-data", strlen("multipart/form-data")))
	{
		nRtn = 2;
	}
	else
	{
		nRtn = 0;
	}

	return nRtn;
}

BOOLEAN httpGetProperty(PHTTP_REQUEST pHttp)
{
	int n = 0;
	int m = 0;
	int i=0;
	PUCHAR pValue;
	UCHAR ch;

	enum {
		sw_name = 0,
		sw_value,
		sw_almost_done
	} state;



	n = httpMatchName(pHttp->HeaderName.Start, pHttp->HeaderName.End - pHttp->HeaderName.Start);
	switch(n)
	{
	case 1:
		break;
	case 2:
		m = httpMatchValue(pHttp->HeaderValue.Start, pHttp->HeaderValue.End - pHttp->HeaderValue.Start);
		switch(m)
		{
		case 1:
			pHttp->FileType = 1;
			break;
		case 2:
			//提取boundary
			pValue = pHttp->HeaderValue.Start;
			state = sw_name;
			while(pValue < pHttp->HeaderValue.End)
			{
				ch = *pValue;
				switch(state)
				{
				case sw_name:
					if (ch == '=')
					{
						state = sw_value;
					}
					else if (ch == 0x0d || ch == 0x0a || ch == 0)
					{
						state = sw_value;
					}
					break;
				case sw_value:
					if (ch == 0x0d ||
						ch == 0x0a)
					{
						state = sw_almost_done;
						break;
					}
					else
					{
						pHttp->Boundary[i]=ch;
						i++;
					}
					break;
				case sw_almost_done:
					goto boundaryend;

				}
				pValue++;
			}
			

boundaryend:
			pHttp->FileType = 2;
			break;
		case 3:
		case 4:
			;
		default:
			;
		}
		break;
	default:
		;
	}


	return 0;
}
int httpParseHeaderLine(PHTTP_REQUEST pHttp)
{
	UCHAR ch;
	PUCHAR pRequestHeader;
	ANSI_STRING msg;

	enum {
		sw_start = 0,
		sw_name,
		sw_space_before_value,
		sw_value,
		sw_space_after_value,
		sw_ignore_line,
		sw_almost_done,
		sw_header_almost_done
	} state;

	pRequestHeader = pHttp->RequestLine.End+2;
	state = pHttp->status;

	while(pRequestHeader <= (pHttp->buffer+pHttp->length))
	{
		ch = *pRequestHeader;
		switch(state)
		{
		case sw_start:
			{
				if (ch == 0x0d)
				{
					pHttp->HeaderLine.End = pRequestHeader;
					state = sw_header_almost_done;
					break;
				}
				if (ch == 0x0a)
				{
					pHttp->HeaderLine.End = pRequestHeader-1;
					goto done;
				}
				if (ch == '/')
				{
					state = sw_ignore_line;
					break;
				}

				pHttp->HeaderLine.Start = pRequestHeader;
				pHttp->HeaderName.Start = pRequestHeader;
				state = sw_name;
				break;
			}
		case sw_name:
			{
				if (ch == ':')
				{
					pHttp->HeaderName.End = pRequestHeader;
					state = sw_space_before_value;
				}
				if (ch == 0x0d)
				{
					state = sw_almost_done;
				}
				if (ch == 0x0a)
				{
					goto done;
				}
				break;
			}
		case sw_space_before_value:
			{
				switch (ch)
				{
					case ' ':
						break;
					case CR:
						state = sw_almost_done;
						break;
					case LF:
						goto done;
					case '\0':
						return HTTP_PARSE_INVALID_HEADER;
					default:
						pHttp->HeaderValue.Start = pRequestHeader;
						state = sw_value;
						break;
				}

				break;
			}
		case sw_value:
			{
				switch(ch)
				{
				case 0x0d:
					pHttp->HeaderValue.End = pRequestHeader;
					state = sw_almost_done;
					break;
				case 0x0a:
					goto done;
					break;
				case ' ':
					state = sw_space_after_value;
					break;
				}
				break;
			}
		case sw_space_after_value:
			{
				switch (ch) 
				{
					case ' ':
						break;
					case 0x0d:
						state = sw_almost_done;
						break;
					case 0x0a:
						goto done;
					case '\0':
						return HTTP_PARSE_INVALID_HEADER;
					default:
						state = sw_value;
						break;
				}

				break;
			}
		case sw_ignore_line:
			{
				switch (ch) 
				{
				case 0x0a:
					state = sw_start;
					break;
				default:
					break;
				}
				break;
			}
		case sw_almost_done://解析完一个请求行
			{
				switch (ch) 
				{
					case 0x0a:
						//goto done;
					case 0x0d:
						{
							pHttp->HeaderLine.End = pRequestHeader;

							//打印下完成的name和value
							/*msg.Buffer = pHttp->HeaderName.Start;
							msg.MaximumLength = msg.Length = pHttp->HeaderName.End - pHttp->HeaderName.Start;
							KdPrint(("name:%Z ", &msg));

							msg.Buffer = pHttp->HeaderValue.Start;
							msg.MaximumLength = msg.Length = pHttp->HeaderValue.End - pHttp->HeaderValue.Start;
							KdPrint(("value:%Z\n", &msg));*/


							httpGetProperty(pHttp);

							//开始下一行
							state = sw_start;
						}
						
						break;
					default:
						return HTTP_PARSE_INVALID_HEADER;
				}
				
				break;
			}
		case sw_header_almost_done://整个请求头解析完成
			{
				switch (ch) 
				{
					case 0x0a:
						pRequestHeader++;
						goto done;
					default:
						return HTTP_PARSE_INVALID_HEADER;
				}
				break;
			}
		}
		pRequestHeader++;
	}

done:

	pHttp->Body.Start = pRequestHeader;
	pHttp->Body.End = pHttp->buffer+pHttp->length;
	return 0;
}



int MiMeParseFile(PHTTP_REQUEST pHttp)
{
	CHAR ch;
	int filenamelength = 0;
	PCHAR pBody;
	enum {
		sw_start = 0,
		sw_filename_fi,
		sw_filename_fil,
		sw_filename_file,
		sw_filename_filen,
		sw_filename_filena,
		sw_filename_filenam,
		sw_filename_filename,
		sw_equal_after_filename,
		sw_quotation_after_filename,
		sw_filename,
		sw_enter,
		sw_blank_line,
		sw_file_content,
		sw_new_segment_minus_sign,
		sw_ignore_line,
		sw_almost_done,
		sw_header_almost_done
	} state;

	state = pHttp->MiMeState;
	pBody = pHttp->Body.Start;

	if (pHttp->FileNumber)
	{
		pHttp->File[pHttp->FileNumber-1].FileBuffer = pHttp->Body.Start;
		pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pHttp->Body.End;
	}
	

	while(pBody < pHttp->Body.End)
	{
		ch = *pBody;
		switch(state)
		{
		case sw_start:
			if (ch == 'f')
			{
				filenamelength = 0;
				state = sw_filename_fi;
			}
			break;
		case sw_filename_fi:
			if (ch == 'i')
			{
				state = sw_filename_fil;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_fil:
			if (ch == 'l')
			{
				state = sw_filename_file;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_file:
			if (ch == 'e')
			{
				state = sw_filename_filen;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filen:
			if (ch == 'n')
			{
				state = sw_filename_filena;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filena:
			if (ch == 'a')
			{
				state = sw_filename_filenam;
				break;
			}
			state = sw_start;
			break;
		case sw_filename_filenam:
			if (ch == 'm')
			{
				state = sw_filename_filename;
				break;
			}
			state = sw_start;
		case sw_filename_filename:
			if (ch == 'e')
			{
				state = sw_equal_after_filename;
				break;
			}
			state = sw_start;
			break;
		case sw_equal_after_filename:
			if (ch == '=')
			{
				state = sw_quotation_after_filename;
			}
			else
			{
				state = sw_start;
			}
			break;
		case sw_quotation_after_filename:
			if (ch == '\"')
			{
				state = sw_filename;
				pHttp->File[pHttp->FileNumber].FileName = ExAllocatePoolWithTag(PagedPool, 512, 0);
				pHttp->FileNumber++;
			}
			else
			{
				state = sw_start;
			}
			break;
		case sw_filename:
			if (ch == '\"')
			{
				state = sw_enter;
			}
			else
			{
				if (filenamelength==512)
				{
					filenamelength=0;
				}

				pHttp->File[pHttp->FileNumber-1].FileName[filenamelength] = ch;
				filenamelength++;
			}
			break;
		case sw_enter:
			if (ch == 0x0d)
			{
			}
			if (ch == 0x0a)
			{
				state = sw_blank_line;
			}
			break;
		case sw_blank_line:
			if (ch == 0x0d)
			{
			}
			else if (ch == 0x0a)
			{//更新FileBuffer
				state = sw_file_content;
				pHttp->File[pHttp->FileNumber-1].FileBuffer = pBody+1;
				break;
			}
			else
			{
				state = sw_enter;
			}


			break;
		case sw_file_content:
			//pBody指向文件的内容，开始判断文件结尾
			if (ch == '-')
			{//更新FileBufferEnd
				pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody-2;
				state = sw_new_segment_minus_sign;
				break;
			}
			break;

		case sw_new_segment_minus_sign:
			if (ch == '-')
			{
				if (!_strnicmp(pBody+1, pHttp->Boundary, strlen(pHttp->Boundary) ))
				{//有新节或到整个MIME尾部，重新开始
					pHttp->File[pHttp->FileNumber-1].FileBufferEnd = pBody-3;
					state = sw_start;
				}
				else
				{
					pBody--;
					state = sw_file_content;
				}
			}
			else
			{
				state = sw_file_content;
			}
			break;

		default:
			break;
		}
		pBody++;
	}

	pHttp->MiMeState = state;

	return 0;
}