package org.github.distributedsystem.demo;

import org.apache.commons.cli.BasicParser;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.github.distributedsystem.demo.methods.Delete;
import org.github.distributedsystem.demo.methods.Get;
import org.github.distributedsystem.demo.methods.Put;

import io.atomix.catalyst.transport.Address;
import io.atomix.catalyst.transport.netty.NettyTransport;
import io.atomix.copycat.server.CopycatServer;

public class Cluster {

	private static CopycatServer server = null;

	public void bindServer(String addr) {
		String[] arr = addr.split(":");
		String host = arr[0];
		int port = Integer.parseInt(arr[1]);
		server = CopycatServer.builder(new Address(host, port)).withTransport(new NettyTransport())
				.withStateMachine(KVStateMachine::new).build();
		server.cluster().onJoin(member -> {
			System.out.println(member.address() + " joined the cluster");
		});

		server.cluster().onLeave(member -> {
			System.out.println(member.address() + " left the cluster");
		});

		server.serializer().register(Put.class, 1);
		server.serializer().register(Get.class, 2);
		server.serializer().register(Delete.class, 3);
	}

	public void setup(String bindAddr, String joinAddr, boolean bootstrapped) {
		bindServer(bindAddr);

		if (!bootstrapped && joinAddr != null) {
			String[] arr = joinAddr.split(":");
			String host = arr[0];
			int port = Integer.parseInt(arr[1]);
			// join cluster
			server.join(new Address(host, port)).join();
		} else {
			// bootstrapping a cluster
			server.bootstrap().join();
		}
	}

	public static void main(String[] args) {
		Cluster c = new Cluster();
		Options options = new Options();
		options.addOption("bind", true, "bind address");
		options.addOption("join", true, "join remote address");

		String bindAddr = "localhost:7000";
		String joinAddr = null;
		
		boolean bootstrapped = true;

		BasicParser parser = new BasicParser();
		try {
			CommandLine cmd = parser.parse(options, args);
			if (cmd.hasOption("bind")) {
				bindAddr = cmd.getOptionValue("bind");
			}
			if (cmd.hasOption("join")) {
				joinAddr = cmd.getOptionValue("join");
				bootstrapped = false;
			}
		} catch (ParseException e) {
			e.printStackTrace();
		}

		c.setup(bindAddr, joinAddr, bootstrapped);
	}
}
