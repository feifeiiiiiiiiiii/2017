package org.github.distributedsystem.demo.methods;

import io.atomix.copycat.Command;


public class Delete implements Command<Object> {
    public Object key;

    @Override
    public CompactionMode compaction() {
        return CompactionMode.TOMBSTONE;
    }

    public Delete(Object key) {
        this.key = key;
    }
}
