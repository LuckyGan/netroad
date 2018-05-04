package chow.dan;

import java.awt.Desktop;
import java.io.File;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Display {
	public static void main(String[] args) {
		File logFolder = new File("/home/dan/ns-3-dce/files-5/var/log");

		List<DceLog> logs = new ArrayList<>();
		try {
			for (File folder : logFolder.listFiles(pathname -> pathname.isDirectory())) {
				logs.add(new DceLog(folder));
			}

			Collections.sort(logs);

			String outfile = "/home/dan/dcecmdlog.txt";
			PrintWriter writer = new PrintWriter(outfile, "UTF-8");
			for (DceLog log : logs) {
				writer.println("Time:\t" + log.getTime());
				writer.println("Cmd:\t" + log.getCmd());
				if (!log.getErr().isEmpty()) {
					writer.println("Err:\t");
					for (String line : log.getErr().getLines()) {
						writer.println("\t" + line);
					}
				}

				if (!log.getOut().isEmpty()) {
					writer.println("Out:\t");
					for (String line : log.getOut().getLines()) {
						writer.println("\t" + line);
					}
				}

				writer.println("-------------");
			}

			writer.close();

			Desktop.getDesktop().open(new File(outfile));
		} catch (Exception e) {
			e.printStackTrace();
		} finally {

		}

	}

}
