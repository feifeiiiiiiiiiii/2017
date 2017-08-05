%%{
    machine common;

    CR = "\r";
    LF = "\n";
    CRLF = CR LF;

    action finalize {
        done = 1;
        System.out.println("done");

    }

    action read_size {
        chunk_size *= 10;
        chunk_size += data[p] - '0';
        System.out.println("read chunk size: " + chunk_size);
    }

    action start_reading_size {
        System.out.println("start reading chunk size");
        chunk_bytes_read = 0;
        chunk_size = 0;
    }

    action start_reading_data {
        System.out.println("start reading data");
        hunk_bytes_read = 0;
    }

    action test_len {
        chunk_bytes_read++ < chunk_size
    }

    chunk_size = ([1-9] digit*) >start_reading_size $read_size
               ;

    chunk_data_octet = any when test_len
                     ;

    chunk_data = chunk_data_octet+;

    action read_chunk {
        chunks_read++;
        System.out.println("have read chunk " + chunks_read);
    }

    action check_data_complete {
        chunk_bytes_read == chunk_size + 1
    }

    trailer = CRLF @read_chunk
            ;

    chunk = "$" "0"+ CRLF trailer
          | "$-" digit+ trailer
          | "$" chunk_size CRLF chunk_data CR when check_data_complete LF @read_chunk
          ;

    single_line_reply = [:\+\-] (any* -- CRLF) CRLF;
}%%
