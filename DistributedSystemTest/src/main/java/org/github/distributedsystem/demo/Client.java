package org.github.distributedsystem.demo;

import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutionException;

import org.github.distributedsystem.demo.methods.Delete;
import org.github.distributedsystem.demo.methods.Get;
import org.github.distributedsystem.demo.methods.Put;

import io.atomix.AtomixClient;
import io.atomix.catalyst.transport.Address;
import io.atomix.catalyst.transport.netty.NettyTransport;
import io.atomix.copycat.client.CopycatClient;

public class Client {

	public static void main(String[] args) throws InterruptedException, ExecutionException {
		CopycatClient client = CopycatClient.builder().withTransport(new NettyTransport()).build();
		List<Address> cluster = Arrays.asList(new Address("localhost", 7001), new Address("localhost", 7000));
		client.serializer().register(Put.class, 1);
		client.serializer().register(Get.class, 2);
		client.serializer().register(Delete.class, 3);
		client.connect(cluster).join();

		String key = "hello";
		String value = "value";
		
		if(client.state() == CopycatClient.State.CLOSED) {
            throw new RuntimeException("Client not connected to server");
        } else {
        	//client.submit(new Put(key, value));
        	System.out.println("put value " + client.submit(new Get(key)).get());
        }
		client.close();
		
	}
}
