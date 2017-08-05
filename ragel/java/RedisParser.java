
// line 1 "RedisParser.rl"
import java.util.*;


// line 13 "RedisParser.rl"



public class RedisParser {
    private int chunk_size;
    private int chunk_data;
    private int chunk_bytes_read;
    private int chunks_read;
    private int chunk_count;

    private int state = -1;
    private int offset = 0;
    private byte[] data = "$-1\r\n-ERR not found command\r\n$10\r\n0123456789\r\n*5\r\n:1\r\n:2\r\n:3\r\n:4\r\n$6\r\nfoobar\r\n".getBytes();

    
// line 23 "RedisParser.java"
private static byte[] init__reply_actions_0()
{
	return new byte [] {
	    0,    1,    0,    1,    1,    1,    4,    1,    6,    1,    7,    2,
	    2,    1,    2,    3,    0,    2,    3,    7,    2,    5,    6
	};
}

private static final byte _reply_actions[] = init__reply_actions_0();


private static short[] init__reply_cond_offsets_0()
{
	return new short [] {
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	    1,    4,    7,    7,    7,    7,    7,    7,    7,    7,   11,   14,
	   15,   17,   18,   21,   26,   28,   29,   30,   32,   33,   34,   37,
	   42,   50,   55,   60,   65,   70,   73,   74,   79,   82,   87,   92,
	   97,   98,  103,  103,  103,  103,  106,  110,  121,  126,  137
	};
}

private static final short _reply_cond_offsets[] = init__reply_cond_offsets_0();


private static byte[] init__reply_cond_lengths_0()
{
	return new byte [] {
	    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	    3,    3,    0,    0,    0,    0,    0,    0,    0,    4,    3,    1,
	    2,    1,    3,    5,    2,    1,    1,    2,    1,    1,    3,    5,
	    8,    5,    5,    5,    5,    3,    1,    5,    3,    5,    5,    5,
	    1,    5,    0,    0,    0,    3,    4,   11,    5,   11,    9
	};
}

private static final byte _reply_cond_lengths[] = init__reply_cond_lengths_0();


private static int[] init__reply_cond_keys_0()
{
	return new int [] {
	    0,65535,    0,   12,   13,   13,   14,65535,    0,   12,   13,   13,
	   14,65535,   36,   36,   43,   43,   45,   45,   58,   58,   45,   45,
	   48,   48,   49,   57,   48,   57,   13,   13,   48,   57,   10,   10,
	    0,   12,   13,   13,   14,65535,    0,    9,   10,   10,   11,   12,
	   13,   13,   14,65535,   13,   13,   48,   48,   10,   10,   13,   13,
	   13,   13,   48,   57,   10,   10,    0,65535,    0,   12,   13,   13,
	   14,65535,    0,    9,   10,   10,   11,   12,   13,   13,   14,65535,
	    0,   12,   13,   13,   14,   44,   45,   45,   46,   47,   48,   48,
	   49,   57,   58,65535,    0,   12,   13,   13,   14,   47,   48,   57,
	   58,65535,    0,   12,   13,   13,   14,   47,   48,   57,   58,65535,
	    0,   12,   13,   13,   14,   47,   48,   48,   49,65535,    0,    9,
	   10,   10,   11,   12,   13,   13,   14,65535,    0,   12,   13,   13,
	   14,65535,   10,   10,    0,    9,   10,   10,   11,   12,   13,   13,
	   14,65535,    0,   12,   13,   13,   14,65535,    0,    9,   10,   10,
	   11,   12,   13,   13,   14,65535,    0,   12,   13,   13,   14,   47,
	   48,   57,   58,65535,    0,    9,   10,   10,   11,   12,   13,   13,
	   14,65535,   10,   10,    0,    9,   10,   10,   11,   12,   13,   13,
	   14,65535,    0,   12,   13,   13,   14,65535,   36,   36,   43,   43,
	   45,   45,   58,   58,    0,   12,   13,   13,   14,   35,   36,   36,
	   37,   42,   43,   43,   44,   44,   45,   45,   46,   57,   58,   58,
	   59,65535,   13,   13,   36,   36,   43,   43,   45,   45,   58,   58,
	    0,   12,   13,   13,   14,   35,   36,   36,   37,   42,   43,   43,
	   44,   44,   45,   45,   46,   57,   58,   58,   59,65535,    0,   35,
	   36,   36,   37,   42,   43,   43,   44,   44,   45,   45,   46,   57,
	   58,   58,   59,65535,    0
	};
}

private static final int _reply_cond_keys[] = init__reply_cond_keys_0();


private static byte[] init__reply_cond_spaces_0()
{
	return new byte [] {
	    0,    0,    3,    0,    0,    3,    0,    2,    2,    2,    2,    2,
	    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
	    2,    2,    2,    2,    2,    2,    2,    2,    2,    4,    4,    5,
	    4,    4,    4,    4,    5,    4,    4,    5,    4,    4,    4,    4,
	    4,    4,    4,    5,    4,    4,    4,    4,    5,    4,    4,    4,
	    4,    5,    4,    4,    4,    4,    4,    4,    5,    4,    4,    5,
	    4,    2,    4,    4,    4,    5,    4,    4,    5,    4,    4,    4,
	    4,    5,    4,    4,    5,    4,    4,    4,    4,    4,    4,    5,
	    4,    2,    4,    4,    4,    5,    4,    0,    3,    0,    2,    2,
	    2,    2,    4,    5,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    2,    2,    2,    2,    2,    4,    5,    4,    4,    4,    4,
	    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
	    4,    4,    0
	};
}

private static final byte _reply_cond_spaces[] = init__reply_cond_spaces_0();


private static short[] init__reply_key_offsets_0()
{
	return new short [] {
	    0,    0,    5,    9,   11,   14,   15,   17,   18,   19,   22,   23,
	   25,   32,   41,   45,   46,   47,   48,   50,   53,   54,   58,   62,
	   64,   67,   68,   71,   75,   77,   78,   79,   82,   83,   85,   92,
	  101,  118,  131,  145,  155,  164,  172,  173,  182,  194,  208,  222,
	  230,  231,  240,  241,  243,  243,  250,  254,  269,  274,  290
	};
}

private static final short _reply_key_offsets[] = init__reply_key_offsets_0();


private static int[] init__reply_trans_keys_0()
{
	return new int [] {
	   36,   42,   43,   45,   58,   45,   48,   49,   57,   48,   57,   13,
	   48,   57,   10,   13,   48,   10,   13,   13,   48,   57,   10,131072,
	196607,393229,458765,524301,131072,131084,131086,196607,65546,131082,393229,458765,
	524301,131072,131084,131086,196607,   45,   48,   49,   57,   49,   13,   10,
	   13,   48,   13,   48,   57,   10,1441828,1441835,1441837,1441850,1441837,1441840,
	1441841,1441849,1441840,1441849,1441805,1441840,1441849,1441802,1441805,1441792,1507327,1441802,
	1441805,1441792,1507327,1441805,1441840,1441802,1441805,1441805,1441840,1441849,1441802,786432,
	851967,1179661,1245197,1310733,786432,786444,786446,851967,720906,786442,1179661,1245197,
	1310733,786432,786444,786446,851967,720941,720944,786477,786480,1179661,1245197,1310733,
	720945,720953,786432,786444,786446,786479,786481,786489,786490,851967,1179661,1245197,
	1310733,720944,720953,786432,786444,786446,786479,786480,786489,786490,851967,1114125,
	1179661,1245197,1310733,720944,720953,786432,786444,786446,786479,786480,786489,786490,
	851967,720944,786480,1114125,1179661,1245197,1310733,786432,786444,786446,851967,720906,
	786442,1179661,1245197,1310733,786432,786444,786446,851967,1114125,1179661,1245197,1310733,
	786432,786444,786446,851967,1441802,720906,786442,1179661,1245197,1310733,786432,786444,
	786446,851967,1114125,1179661,1245197,1310733,720896,720908,720910,786431,786432,786444,
	786446,851967,720906,786442,1114125,1179661,1245197,1310733,720896,720908,720910,786431,
	786432,786444,786446,851967,1114125,1179661,1245197,1310733,720944,720953,786432,786444,
	786446,786479,786480,786489,786490,851967,720906,1179661,1245197,1310733,786432,786444,
	786446,851967,1441802,720906,786442,1179661,1245197,1310733,786432,786444,786446,851967,
	   13,   10,   13,393229,458765,524301,131072,131084,131086,196607,1441828,1441835,
	1441837,1441850,720932,720939,720941,720954,786468,786475,786477,786490,1179661,1245197,
	1310733,786432,786444,786446,851967,1441805,1441828,1441835,1441837,1441850,720932,720939,
	720941,720954,786468,786475,786477,786490,1114125,1179661,1245197,1310733,786432,786444,
	786446,851967,720932,720939,720941,720954,786468,786475,786477,786490,786432,851967,
	    0
	};
}

private static final int _reply_trans_keys[] = init__reply_trans_keys_0();


private static byte[] init__reply_single_lengths_0()
{
	return new byte [] {
	    0,    5,    2,    0,    1,    1,    2,    1,    1,    1,    1,    0,
	    3,    5,    2,    1,    1,    1,    2,    1,    1,    4,    2,    0,
	    1,    1,    1,    2,    2,    1,    1,    1,    1,    0,    3,    5,
	    7,    3,    4,    6,    5,    4,    1,    5,    4,    6,    4,    4,
	    1,    5,    1,    2,    0,    3,    4,   11,    5,   12,    8
	};
}

private static final byte _reply_single_lengths[] = init__reply_single_lengths_0();


private static byte[] init__reply_range_lengths_0()
{
	return new byte [] {
	    0,    0,    1,    1,    1,    0,    0,    0,    0,    1,    0,    1,
	    2,    2,    1,    0,    0,    0,    0,    1,    0,    0,    1,    1,
	    1,    0,    1,    1,    0,    0,    0,    1,    0,    1,    2,    2,
	    5,    5,    5,    2,    2,    2,    0,    2,    4,    4,    5,    2,
	    0,    2,    0,    0,    0,    2,    0,    2,    0,    2,    1
	};
}

private static final byte _reply_range_lengths[] = init__reply_range_lengths_0();


private static short[] init__reply_index_offsets_0()
{
	return new short [] {
	    0,    0,    6,   10,   12,   15,   17,   20,   22,   24,   27,   29,
	   31,   37,   45,   49,   51,   53,   55,   58,   61,   63,   68,   72,
	   74,   77,   79,   82,   86,   89,   91,   93,   96,   98,  100,  106,
	  114,  127,  136,  146,  155,  163,  170,  172,  180,  189,  200,  210,
	  217,  219,  227,  229,  232,  233,  239,  244,  258,  264,  279
	};
}

private static final short _reply_index_offsets[] = init__reply_index_offsets_0();


private static byte[] init__reply_indicies_0()
{
	return new byte [] {
	    0,    2,    3,    3,    3,    1,    4,    5,    6,    1,    7,    1,
	    8,    7,    1,    9,    1,   10,    5,    1,   11,    1,    8,    1,
	   12,   13,    1,   14,    1,   15,    1,   15,    8,   16,   15,   15,
	    1,    9,   17,   15,    8,   16,   15,   15,    1,   18,   19,   20,
	    1,   21,    1,   22,    1,   23,    1,   22,   19,    1,   24,   25,
	    1,   26,    1,   27,   28,   28,   28,    1,   29,   30,   31,    1,
	   32,    1,   33,   32,    1,   34,    1,   35,   28,    1,   34,   35,
	   28,    1,   36,   30,    1,   37,    1,   33,    1,   38,   39,    1,
	   40,    1,   41,    1,   41,   33,   42,   41,   41,    1,   34,   43,
	   41,   33,   42,   41,   41,    1,   29,   30,   44,   45,   41,   33,
	   42,   31,   41,   41,   46,   41,    1,   41,   33,   42,   32,   41,
	   41,   47,   41,    1,   33,   42,   33,   42,   32,   41,   41,   47,
	   41,    1,   30,   45,   36,   48,   49,   50,   41,   41,    1,   37,
	   51,   41,   33,   42,   41,   41,    1,   33,   42,   33,   42,   41,
	   41,    1,   52,    1,   52,   53,   41,   33,   42,   41,   41,    1,
	   35,   55,   35,   55,   28,   28,   54,   54,    1,   34,   43,   35,
	   55,   35,   55,   28,   28,   54,   54,    1,   38,   57,   58,   59,
	   39,   41,   41,   56,   41,    1,   40,   41,   33,   42,   41,   41,
	    1,   60,    1,   60,   43,   41,   33,   42,   41,   41,    1,   61,
	    3,   62,   61,    3,    1,   15,    8,   16,   15,   15,    1,   27,
	   28,   28,   28,    1,   27,   28,   28,   28,   63,   54,   54,   54,
	   41,   33,   42,   41,   41,    1,   33,   27,   28,   28,   28,    1,
	   27,   28,   28,   28,   63,   54,   54,   54,   33,   42,   33,   42,
	   41,   41,    1,   27,   28,   28,   28,   63,   54,   54,   54,   41,
	    1,    0
	};
}

private static final byte _reply_indicies[] = init__reply_indicies_0();


private static byte[] init__reply_trans_targs_0()
{
	return new byte [] {
	    2,    0,   14,   50,    3,    6,    9,    4,    5,   52,    7,    8,
	   10,    9,   11,   12,   13,   53,   15,   18,   19,   16,   17,   52,
	   20,   19,   21,   22,   26,   23,   28,   31,   24,   25,   54,   27,
	   29,   30,   32,   31,   33,   34,   35,   55,   37,   39,   46,   38,
	   40,   42,   43,   41,   56,   57,   44,   45,   46,   47,   48,   49,
	   58,   51,   52,   36
	};
}

private static final byte _reply_trans_targs[] = init__reply_trans_targs_0();


private static byte[] init__reply_trans_actions_0()
{
	return new byte [] {
	    0,    0,    0,    0,    0,    0,   11,    0,    0,   14,    0,    0,
	    0,    3,    0,    0,    0,   14,    0,    0,   20,    0,    0,    9,
	    0,    7,    5,    0,    0,    0,    0,   11,    0,    0,   17,    0,
	    0,    0,    0,    3,    0,    0,    0,   17,    0,    0,   11,    0,
	    0,    0,    0,    0,   17,   17,    0,    0,    3,    0,    0,    0,
	   17,    0,    1,    0
	};
}

private static final byte _reply_trans_actions[] = init__reply_trans_actions_0();


static final int reply_start = 1;
static final int reply_first_final = 52;
static final int reply_error = 0;

static final int reply_en_main = 1;


// line 28 "RedisParser.rl"

    public RedisParser() {
    }
    
    public boolean exec() {
    
        int p = offset;
        int pe = data.length;
        int ts, te, act, cs;
        int eof = pe;
        int done = 0;
        
        if(state == -1) {
            
// line 296 "RedisParser.java"
	{
	cs = reply_start;
	}

// line 42 "RedisParser.rl"
            state = cs;
        } else {
            cs = state;
        }

        
// line 308 "RedisParser.java"
	{
	int _klen;
	int _trans = 0;
	int _widec;
	int _acts;
	int _nacts;
	int _keys;
	int _goto_targ = 0;

	_goto: while (true) {
	switch ( _goto_targ ) {
	case 0:
	if ( p == pe ) {
		_goto_targ = 4;
		continue _goto;
	}
	if ( cs == 0 ) {
		_goto_targ = 5;
		continue _goto;
	}
case 1:
	_widec = data[p];
	_keys = _reply_cond_offsets[cs]*2
;	_klen = _reply_cond_lengths[cs];
	if ( _klen > 0 ) {
		int _lower = _keys
;		int _mid;
		int _upper = _keys + (_klen<<1) - 2;
		while (true) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( _widec < _reply_cond_keys[_mid] )
				_upper = _mid - 2;
			else if ( _widec > _reply_cond_keys[_mid+1] )
				_lower = _mid + 2;
			else {
				switch ( _reply_cond_spaces[_reply_cond_offsets[cs] + ((_mid - _keys)>>1)] ) {
	case 0: {
		_widec = 65536 + (data[p] - 0);
		if ( 
// line 31 "common.rl"

        chunk_bytes_read++ < chunk_size
     ) _widec += 65536;
		break;
	}
	case 1: {
		_widec = 196608 + (data[p] - 0);
		if ( 
// line 48 "common.rl"

        chunk_bytes_read == chunk_size + 1
     ) _widec += 65536;
		break;
	}
	case 2: {
		_widec = 1376256 + (data[p] - 0);
		if ( 
// line 6 "multi_bulk_reply.rl"

        chunks_read < chunk_count
     ) _widec += 65536;
		break;
	}
	case 3: {
		_widec = 327680 + (data[p] - 0);
		if ( 
// line 31 "common.rl"

        chunk_bytes_read++ < chunk_size
     ) _widec += 65536;
		if ( 
// line 48 "common.rl"

        chunk_bytes_read == chunk_size + 1
     ) _widec += 131072;
		break;
	}
	case 4: {
		_widec = 589824 + (data[p] - 0);
		if ( 
// line 31 "common.rl"

        chunk_bytes_read++ < chunk_size
     ) _widec += 65536;
		if ( 
// line 6 "multi_bulk_reply.rl"

        chunks_read < chunk_count
     ) _widec += 131072;
		break;
	}
	case 5: {
		_widec = 851968 + (data[p] - 0);
		if ( 
// line 31 "common.rl"

        chunk_bytes_read++ < chunk_size
     ) _widec += 65536;
		if ( 
// line 48 "common.rl"

        chunk_bytes_read == chunk_size + 1
     ) _widec += 131072;
		if ( 
// line 6 "multi_bulk_reply.rl"

        chunks_read < chunk_count
     ) _widec += 262144;
		break;
	}
				}
				break;
			}
		}
	}

	_match: do {
	_keys = _reply_key_offsets[cs];
	_trans = _reply_index_offsets[cs];
	_klen = _reply_single_lengths[cs];
	if ( _klen > 0 ) {
		int _lower = _keys;
		int _mid;
		int _upper = _keys + _klen - 1;
		while (true) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( _widec < _reply_trans_keys[_mid] )
				_upper = _mid - 1;
			else if ( _widec > _reply_trans_keys[_mid] )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				break _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _reply_range_lengths[cs];
	if ( _klen > 0 ) {
		int _lower = _keys;
		int _mid;
		int _upper = _keys + (_klen<<1) - 2;
		while (true) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( _widec < _reply_trans_keys[_mid] )
				_upper = _mid - 2;
			else if ( _widec > _reply_trans_keys[_mid+1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				break _match;
			}
		}
		_trans += _klen;
	}
	} while (false);

	_trans = _reply_indicies[_trans];
	cs = _reply_trans_targs[_trans];

	if ( _reply_trans_actions[_trans] != 0 ) {
		_acts = _reply_trans_actions[_trans];
		_nacts = (int) _reply_actions[_acts++];
		while ( _nacts-- > 0 )
	{
			switch ( _reply_actions[_acts++] )
			{
	case 0:
// line 8 "common.rl"
	{
        done = 1;
        System.out.println("done");

    }
	break;
	case 1:
// line 14 "common.rl"
	{
        chunk_size *= 10;
        chunk_size += data[p] - '0';
        System.out.println("read chunk size: " + chunk_size);
    }
	break;
	case 2:
// line 20 "common.rl"
	{
        System.out.println("start reading chunk size");
        chunk_bytes_read = 0;
        chunk_size = 0;
    }
	break;
	case 3:
// line 43 "common.rl"
	{
        chunks_read++;
        System.out.println("have read chunk " + chunks_read);
    }
	break;
	case 4:
// line 10 "multi_bulk_reply.rl"
	{
        System.out.println("start reading bulk");
        chunks_read = 0;
    }
	break;
	case 5:
// line 15 "multi_bulk_reply.rl"
	{
        System.out.println("start reading bulk count");
        chunk_count = 0;
    }
	break;
	case 6:
// line 20 "multi_bulk_reply.rl"
	{
        chunk_count *= 10;
        chunk_count += data[p] - '0';
        System.out.println("chunk count: " + chunk_count);
    }
	break;
	case 7:
// line 26 "multi_bulk_reply.rl"
	{
        System.out.println("finalize multi bulks");

        if (chunks_read == chunk_count) {
            System.out.println("done multi bunlk reading!");
            done = 1;
        }
    }
	break;
// line 551 "RedisParser.java"
			}
		}
	}

case 2:
	if ( cs == 0 ) {
		_goto_targ = 5;
		continue _goto;
	}
	if ( ++p != pe ) {
		_goto_targ = 1;
		continue _goto;
	}
case 4:
case 5:
	}
	break; }
	}

// line 48 "RedisParser.rl"
        
        state = cs;
        
        System.out.println("state = " + cs + "  " + reply_error);

        if(done != 1 && cs == reply_error) {
            System.out.println("invalid protocal");
            state = -2;
            return false;
        }
            
        if(done == 1) {
            System.out.println("result = " + new String(Arrays.copyOfRange(data, offset, p)));
            offset = p;    
            state = -1;
            return true;
        }
        return false;
    }

    public static void main(String[] argc) {
        RedisParser rd = new RedisParser();
        while(rd.exec() == true) {
            System.out.println("---------");
        };
    }
};