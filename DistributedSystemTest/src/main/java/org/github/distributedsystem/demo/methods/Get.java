package org.github.distributedsystem.demo.methods;

import io.atomix.copycat.Query;

public class Get implements Query<Object> {
	public Object key;

	@Override
	public ConsistencyLevel consistency() {
		return ConsistencyLevel.LINEARIZABLE_LEASE;
	}

	public Get(Object key) {
		this.key = key;
	}
}
