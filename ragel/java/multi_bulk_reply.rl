%%{
    machine multi_bulk_reply;

    include common "common.rl";

    action test_chunk_count {
        chunks_read < chunk_count
    }

    action start_reading_chunk {
        System.out.println("start reading bulk");
        chunks_read = 0;
    }

    action start_reading_count {
        System.out.println("start reading bulk count");
        chunk_count = 0;
    }

    action read_count {
        chunk_count *= 10;
        chunk_count += data[p] - '0';
        System.out.println("chunk count: " + chunk_count);
    }

    action multi_bulk_finalize {
        System.out.println("finalize multi bulks");

        if (chunks_read == chunk_count) {
            System.out.println("done multi bunlk reading!");
            done = 1;
        }
    }

    reply = single_line_reply @read_chunk
          | chunk
          ;

    protected_chunk = reply when test_chunk_count
                    ;

    chunk_count = ([1-9] digit*) >start_reading_count $read_count
                ;

    multi_bulk_reply = "*" "-1" CRLF @multi_bulk_finalize
                     | "*" "0"+ CRLF @multi_bulk_finalize
                     | "*" chunk_count CRLF @start_reading_chunk
                        protected_chunk+
                        @multi_bulk_finalize
                     ;

}%%
