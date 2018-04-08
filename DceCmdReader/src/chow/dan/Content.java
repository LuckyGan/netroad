package chow.dan;

import java.util.ArrayList;
import java.util.List;

public class Content {
	private List<String> lines;

	public Content() {
		lines = new ArrayList<>();
	}

	public void append(String line) {
		lines.add(line);
	}

	public List<String> getLines() {
		return lines;
	}

	public boolean isEmpty() {
		return lines.isEmpty();
	}

	@Override
	public String toString() {
		return lines.toString();
	}
}
