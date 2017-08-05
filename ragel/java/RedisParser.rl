import java.util.*;

%%{
    machine reply;

    include multi_bulk_reply "multi_bulk_reply.rl";

        main := single_line_reply @finalize
          | chunk @finalize
          | multi_bulk_reply
          ;

}%%


public class RedisParser {
    private int chunk_size;
    private int chunk_data;
    private int chunk_bytes_read;
    private int chunks_read;
    private int chunk_count;

    private int state = -1;
    private int offset = 0;
    private byte[] data = "$-1\r\n-ERR not found command\r\n$10\r\n0123456789\r\n*5\r\n:1\r\n:2\r\n:3\r\n:4\r\n$6\r\nfoobar\r\n".getBytes();

    %% write data;

    public RedisParser() {
    }
    
    public boolean exec() {
    
        int p = offset;
        int pe = data.length;
        int ts, te, act, cs;
        int eof = pe;
        int done = 0;
        
        if(state == -1) {
            %% write init;
            state = cs;
        } else {
            cs = state;
        }

        %% write exec;
        
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