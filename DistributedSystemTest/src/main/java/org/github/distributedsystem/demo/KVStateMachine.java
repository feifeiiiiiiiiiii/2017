package org.github.distributedsystem.demo;

import java.util.HashMap;
import java.util.Map;

import org.github.distributedsystem.demo.methods.Delete;
import org.github.distributedsystem.demo.methods.Get;
import org.github.distributedsystem.demo.methods.Put;

import io.atomix.copycat.server.StateMachine;
import io.atomix.copycat.server.Commit;

public class KVStateMachine extends StateMachine {
	private Map<Object, Commit> storage = new HashMap<>();

	public Object put(Commit<Put> commit) {
		Commit<Put> put = storage.put(commit.operation().key, commit);
		return put == null ? null : put.operation().value;
	}

	public Object get(Commit<Get> commit) {
		try {
			Commit<Put> put = storage.get(commit.operation().key);
			return put == null ? null : put.operation().value;
		} finally {
			commit.release();
		}
	}

	public Object delete(Commit<Delete> commit) {
		Commit<Put> put = null;
		try {
			put = storage.remove(commit.operation().key);
			return put == null ? null : put.operation().value;
		} finally {
			if (put != null)
				put.release();
			commit.release();
		}
	}
}
