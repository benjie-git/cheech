import SwiftUI

@main
struct CheechApp: App {
	@StateObject private var model = SessionModel()

	var body: some Scene {
		WindowGroup {
			RootView()
				.environmentObject(model)
				.onOpenURL { model.handle(url: $0) }
		}
	}
}
