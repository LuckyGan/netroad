package chow.dan;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class DceLog implements Comparable<DceLog> {

	private double time;
	private String cmd;
	private Content err;
	private Content out;

	public DceLog(File folder) {
		try {
			setTime(folder);
			setCmd(folder);
			setErr(folder);
			setOut(folder);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public double getTime() {
		return time;
	}

	public String getCmd() {
		return cmd;
	}

	public Content getErr() {
		return err;
	}

	public Content getOut() {
		return out;
	}

	@Override
	public int compareTo(DceLog other) {
		return new Double(this.time).compareTo(new Double(other.time));
	}

	private void setTime(File folder) throws IOException {
		BufferedReader br = new BufferedReader(new FileReader(new File(folder + "/status")));
		String line = br.readLine();
		time = Double.parseDouble(line.substring(line.indexOf('+') + 1, line.indexOf("ns"))) / 1e9;
		br.close();
	}

	private void setCmd(File folder) throws IOException {
		BufferedReader br = new BufferedReader(new FileReader(new File(folder + "/cmdline")));
		cmd = br.readLine();
		br.close();
	}

	private void setErr(File folder) throws IOException {
		err = new Content();

		BufferedReader br = new BufferedReader(new FileReader(new File(folder + "/stderr")));
		String line;
		while ((line = br.readLine()) != null) {
			err.append(line);
		}

		br.close();
	}

	private void setOut(File folder) throws IOException {
		out = new Content();

		BufferedReader br = new BufferedReader(new FileReader(new File(folder + "/stdout")));
		String line;
		while ((line = br.readLine()) != null) {
			out.append(line);
		}

		br.close();
	}

}
