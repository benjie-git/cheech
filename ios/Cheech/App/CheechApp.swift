import SwiftUI

@main
struct CheechApp: App {
	@StateObject private var model = SessionModel()
	@Environment(\.scenePhase) private var scenePhase

	var body: some Scene {
		WindowGroup {
			RootView()
				.environmentObject(model)
				.onOpenURL { model.handle(url: $0) }
				.onChange(of: scenePhase) { _, phase in
					// Persist any in-progress all-local game so it can be
					// resumed after the app is suspended or killed.
					if phase != .active {
						model.persistLocalGame()
					}
				}
		}
	}
}
